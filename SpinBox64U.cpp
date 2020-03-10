// The MIT License( MIT )
//
// Copyright( c ) 2020 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "SpinBox64U.h"

#include <QLineEdit>
#include <QDebug>
#include <QEvent>
#include <QStyle>
#include <QStyleOption>
#include <QApplication>

enum class EEmitPolicy
{
    eEmitIfChanged,
    eAlwaysEmit,
    eNeverEmit
};

class CSpinBox64UImpl
{
public:
    CSpinBox64UImpl( CSpinBox64U * parent ) : fParent( parent ){};

    virtual uint64_t valueFromText( const QString& text ) const;
    virtual QString textFromValue( uint64_t val ) const;
    void setValue( uint64_t value, EEmitPolicy ep, bool update = true );
    QVariant validateAndInterpret( QString& input, int& pos, QValidator::State& state ) const;
    QVariant calculateAdaptiveDecimalStep( int steps ) const;
    void updateEdit();
    QString stripped( const QString& text, int* pos = 0 ) const;
    void clearCache() const;
    uint64_t bound( const uint64_t& val, const uint64_t& old = 0, int steps = 0 ) const;
    bool specialValue() const;
    void emitSignals( EEmitPolicy ep, const uint64_t& old );
    QString longestAllowedString() const;
    void setRange( uint64_t min, uint64_t max );
    QSize sizeHint() const;
    QAbstractSpinBox::StepEnabled stepEnabled() const;

    uint64_t fValue{ 0 };
    uint64_t fMinimum{ 0 };
    uint64_t fMaximum{ 99 };
    uint64_t fSingleStep{ 1 };
    int fDisplayBase{ 10 };
    QString fPrefix;
    QString fSuffix;
    mutable QSize fCachedLongestAllowedString;
    mutable QSize fCachedMinimumSizeHint;
    QString fSpecialValueText;

    mutable QString fCachedText;
    mutable uint64_t fCachedValue;
    mutable QValidator::State fCachedState;
    mutable QString fCachedMaxAllowedString;

    bool fWrapping{ false };
    QAbstractSpinBox::StepType fStepType{ QAbstractSpinBox::StepType::DefaultStepType };
    bool fIgnoreCursorPositionChanged{ false };

    CSpinBox64U * fParent{ nullptr };
};

void CSpinBox64UImpl::setValue( uint64_t value, EEmitPolicy ep, bool update )
{
    auto old = fValue;
    fValue = bound( value );
    if ( update )
    {
        updateEdit();
    }
    fParent->update();

    if ( ep == EEmitPolicy::eAlwaysEmit || ( ep == EEmitPolicy::eEmitIfChanged && old != value ) )
    {
        emitSignals( ep, old );
    }
}

void CSpinBox64UImpl::emitSignals( EEmitPolicy ep, const uint64_t& old )
{
    if ( ep != EEmitPolicy::eNeverEmit )
    {
        if ( ep == EEmitPolicy::eAlwaysEmit || fValue != old )
        {
            emit fParent->valueChanged( fParent->lineEdit()->displayText() );
            emit fParent->valueChanged( fValue );
        }
    }
}

QString CSpinBox64UImpl::stripped( const QString& t, int* pos ) const
{
    QStringRef text( &t );
    if ( fSpecialValueText.size() == 0 || text != fSpecialValueText )
    {
        int from = 0;
        int size = text.size();
        bool changed = false;
        if ( fPrefix.size() && text.startsWith( fPrefix ) )
        {
            from += fPrefix.size();
            size -= from;
            changed = true;
        }
        if ( fSuffix.size() && text.endsWith( fSuffix ) )
        {
            size -= fSuffix.size();
            changed = true;
        }
        if ( changed )
            text = text.mid( from, size );
    }

    const int s = text.size();
    text = text.trimmed();
    if ( pos )
        ( *pos ) -= ( s - text.size() );
    return text.toString();

}

void CSpinBox64UImpl::clearCache() const
{
    fCachedText.clear();
    fCachedValue = 0;
    fCachedMaxAllowedString = QString();
    fCachedState = QValidator::Acceptable;
}

uint64_t CSpinBox64UImpl::bound( const uint64_t& val, const uint64_t& old, int steps ) const
{
    uint64_t v = val;
    if ( !fWrapping || ( steps == 0 ) || ( old == 0 ) )
    {
        if ( v < fMinimum )
        {
            v = fWrapping ? fMaximum : fMinimum;
        }
        if ( v > fMaximum )
        {
            v = fWrapping ? fMinimum : fMaximum;
        }
    }
    else
    {
        const bool wasMin = old == fMinimum;
        const bool wasMax = old == fMaximum;
        const bool oldcmp = v > old;
        const bool maxcmp = v > fMaximum;
        const bool mincmp = v > fMinimum;
        const bool wrapped = ( oldcmp && steps < 0 ) || ( !oldcmp && steps > 0 );
        if ( maxcmp )
        {
            v = ( ( wasMax && !wrapped && steps > 0 ) || ( steps < 0 && !wasMin && wrapped ) ) ? fMinimum : fMaximum;
        }
        else if ( wrapped && ( maxcmp || !mincmp ) )
        {
            v = ( ( wasMax && steps > 0 ) || ( !wasMin && steps < 0 ) ) ? fMinimum : fMaximum;
        }
        else if ( !mincmp )
        {
            v = ( !wasMax && !wasMin ? fMinimum : fMaximum );
        }
    }

    return v;
}

bool CSpinBox64UImpl::specialValue() const
{
    return ( fValue == fMinimum && !fSpecialValueText.isEmpty() );
}

uint64_t CSpinBox64UImpl::valueFromText( const QString& text ) const
{
    QString copy = text;
    int pos = fParent->lineEdit()->cursorPosition();
    QValidator::State state = QValidator::Acceptable;
    return validateAndInterpret( copy, pos, state ).toULongLong();
}

QAbstractSpinBox::StepEnabled CSpinBox64UImpl::stepEnabled() const
{
    if ( fParent->isReadOnly() )
        return QAbstractSpinBox::StepEnabledFlag::StepNone;
    if ( fWrapping )
        return QAbstractSpinBox::StepEnabled( QAbstractSpinBox::StepEnabledFlag::StepUpEnabled | QAbstractSpinBox::StepEnabledFlag::StepDownEnabled );
    QAbstractSpinBox::StepEnabled ret = QAbstractSpinBox::StepEnabledFlag::StepNone;
    if ( fValue < fMaximum )
    {
        ret |= QAbstractSpinBox::StepEnabledFlag::StepUpEnabled;
    }
    if ( fValue > fMinimum )
    {
        ret |= QAbstractSpinBox::StepEnabledFlag::StepDownEnabled;
    }
    return ret;
}

QVariant CSpinBox64UImpl::validateAndInterpret( QString& input, int& pos, QValidator::State& state ) const
{
    if ( fCachedText == input && !input.isEmpty() )
    {
        state = fCachedState;
        return fCachedValue;
    }

    QString copy = stripped( input, &pos );
    state = QValidator::Acceptable;
    auto num = fMinimum;

    if ( fMaximum != fMinimum && ( copy.isEmpty()
                                   || ( fMinimum < 0 && copy == QLatin1String( "-" ) )
                                   || ( fMaximum >= 0 && copy == QLatin1String( "+" ) ) ) )
    {
        state = QValidator::Intermediate;
    }
    else if ( copy.startsWith( QLatin1Char( '-' ) ) && fMinimum >= 0 )
    {
        state = QValidator::Invalid; // special-case -0 will be interpreted as 0 and thus not be invalid with a range from 0-100
    }
    else
    {
        bool ok = false;
        if ( fDisplayBase != 10 )
        {
            num = copy.toULongLong( &ok, fDisplayBase );
        }
        else
        {
            num = fParent->locale().toULongLong( copy, &ok );
            if ( !ok && ( fMaximum >= 1000 || fMinimum <= -1000 ) )
            {
                const QChar sep = fParent->locale().groupSeparator();
                const QChar doubleSep[ 2 ] = { sep, sep };
                if ( copy.contains( sep ) && !copy.contains( QString( doubleSep, 2 ) ) )
                {
                    QString copy2 = copy;
                    copy2.remove( fParent->locale().groupSeparator() );
                    num = fParent->locale().toULongLong( copy2, &ok );
                }
            }
        }
        if ( !ok )
        {
            state = QValidator::Invalid;
        }
        else if ( num >= fMinimum && num <= fMaximum )
        {
            state = QValidator::Acceptable;
        }
        else if ( fMaximum == fMinimum )
        {
            state = QValidator::Invalid;
        }
        else
        {
            if ( ( num >= 0 && num > fMaximum ) || ( num < 0 && num < fMinimum ) )
            {
                state = QValidator::Invalid;
            }
            else
            {
                state = QValidator::Intermediate;
            }
        }
    }
    if ( state != QValidator::Acceptable )
        num = fMaximum > 0 ? fMinimum : fMaximum;
    input = fPrefix + copy + fSuffix;
    fCachedText = input;
    fCachedState = state;
    fCachedValue = num;

    return fCachedValue;
}

QVariant CSpinBox64UImpl::calculateAdaptiveDecimalStep( int steps ) const
{
    const uint64_t intValue = fValue;
    const uint64_t absValue = intValue;

    if ( absValue < 100 )
        return 1;

    const bool valueNegative = intValue < 0;
    const bool stepsNegative = steps < 0;
    const int signCompensation = ( valueNegative == stepsNegative ) ? 0 : 1;

    const int log = static_cast<int>( std::log10( absValue - signCompensation ) ) - 1;
    return static_cast<uint64_t>( std::pow( 10, log ) );
}

void CSpinBox64UImpl::updateEdit()
{
    const QString newText = specialValue() ? fSpecialValueText : fPrefix + textFromValue( fValue ) + fSuffix;
    if ( newText == fParent->lineEdit()->displayText() )
        return;

    const bool empty = fParent->lineEdit()->text().isEmpty();
    int cursor = fParent->lineEdit()->cursorPosition();
    int selsize = fParent->lineEdit()->selectedText().size();
    const QSignalBlocker blocker( fParent->lineEdit() );
    fParent->lineEdit()->setText( newText );

    if ( !specialValue() )
    {
        cursor = qBound( fPrefix.size(), cursor, fParent->lineEdit()->displayText().size() - fSuffix.size() );

        if ( selsize > 0 )
        {
            fParent->lineEdit()->setSelection( cursor, selsize );
        }
        else
        {
            fParent->lineEdit()->setCursorPosition( empty ? fPrefix.size() : cursor );
        }
    }
    fParent->update();
}

void CSpinBox64UImpl::setRange( uint64_t min, uint64_t max )
{
    clearCache();
    fMinimum = min;
    fMaximum = ( min < max ) ? max : min;
    fCachedLongestAllowedString = QSize();
    fCachedMinimumSizeHint = QSize(); // minimumSizeHint cares about min/max
    fCachedMaxAllowedString = QString();

    if ( !( bound( fValue ) == fValue ) )
    {
        setValue( bound( fValue ), EEmitPolicy::eEmitIfChanged );
    }
    else if ( fValue == fMinimum && !fSpecialValueText.isEmpty() )
    {
        updateEdit();
    }

    fParent->updateGeometry();
}

QString CSpinBox64UImpl::longestAllowedString() const
{
    if ( fCachedMaxAllowedString.isEmpty() )
    {
        QString minStr = textFromValue( fMinimum );
        QString maxStr = textFromValue( fMaximum );
        const QFontMetrics fm( fParent->fontMetrics() );
        if ( fm.horizontalAdvance( minStr ) > fm.horizontalAdvance( maxStr ) )
            fCachedMaxAllowedString = minStr;
        else
            fCachedMaxAllowedString = maxStr;
    }
    return fCachedMaxAllowedString;
}

QSize CSpinBox64UImpl::sizeHint() const
{
    if ( fCachedLongestAllowedString.isEmpty() )
    {
        fParent->ensurePolished();

        const QFontMetrics fm( fParent->fontMetrics() );
        int h = fParent->lineEdit()->sizeHint().height();
        int w = 0;
        QString s;
        QString fixedContent = fPrefix + fSuffix + QLatin1Char( ' ' );
        s = longestAllowedString();
        s += fixedContent;
        w = std::max( w, fm.horizontalAdvance( s ) );

        if ( fSpecialValueText.size() )
        {
            s = fSpecialValueText;
            w = std::max( w, fm.horizontalAdvance( s ) );
        }
        w += 2; // cursor blinking space

        QStyleOptionSpinBox opt;
        fParent->initStyleOption( &opt );
        QSize hint( w, h );
        fCachedLongestAllowedString = fParent->style()->sizeFromContents( QStyle::CT_SpinBox, &opt, hint, fParent ).expandedTo( QApplication::globalStrut() );
    }
    return fCachedLongestAllowedString;
}

QString CSpinBox64UImpl::textFromValue( uint64_t value ) const
{
    QString str;

    if ( fDisplayBase != 10 )
    {
        const QLatin1String prefix = value < 0 ? QLatin1String( "-" ) : QLatin1String();
        str = prefix + QString::number( value, fDisplayBase );
    }
    else
    {
        str = fParent->locale().toString( value );
        if ( !fParent->isGroupSeparatorShown() && ( value >= 1000 || value == INT_MIN ) )
        {
            str.remove( fParent->locale().groupSeparator() );
        }
    }

    return str;
}

Q_DECLARE_METATYPE( uint64_t );

CSpinBox64U::CSpinBox64U( QWidget* parent /*= 0 */ ) :
    QAbstractSpinBox( parent ),
    fImpl( new CSpinBox64UImpl( this ) )
{
    qRegisterMetaType< uint64_t >( "uint64_t" );
    connectLineEdit();
}

CSpinBox64U::~CSpinBox64U()
{
}

uint64_t CSpinBox64U::value() const
{
    return fImpl->fValue;
}

void CSpinBox64U::slotEditorCursorPositionChanged( int oldpos, int newpos )
{
    if ( !lineEdit()->hasSelectedText() && !fImpl->fIgnoreCursorPositionChanged && !fImpl->specialValue() )
    {
        fImpl->fIgnoreCursorPositionChanged = true;

        bool allowSelection = true;
        int pos = -1;
        if ( newpos < fImpl->fPrefix.size() && newpos != 0 )
        {
            if ( oldpos == 0 )
            {
                allowSelection = false;
                pos = fImpl->fPrefix.size();
            }
            else
            {
                pos = oldpos;
            }
        }
        else if ( newpos > lineEdit()->text().size() - fImpl->fSuffix.size()
                  && newpos != lineEdit()->text().size() )
        {
            if ( oldpos == lineEdit()->text().size() )
            {
                pos = lineEdit()->text().size() - fImpl->fSuffix.size();
                allowSelection = false;
            }
            else
            {
                pos = lineEdit()->text().size();
            }
        }
        if ( pos != -1 )
        {
            const int selSize = lineEdit()->selectionStart() >= 0 && allowSelection
                ? ( lineEdit()->selectedText().size()
                    * ( newpos < pos ? -1 : 1 ) ) - newpos + pos
                : 0;

            const QSignalBlocker blocker( lineEdit() );
            if ( selSize != 0 )
            {
                lineEdit()->setSelection( pos - selSize, selSize );
            }
            else
            {
                lineEdit()->setCursorPosition( pos );
            }
        }
        fImpl->fIgnoreCursorPositionChanged = false;
    }
}

void CSpinBox64U::slotEditorTextChanged( const QString& t )
{
    QString tmp = t;
    int pos = lineEdit()->cursorPosition();
    QValidator::State state = validate( tmp, pos );
    if ( state == QValidator::Acceptable )
    {
        auto v = fImpl->valueFromText( tmp );
        fImpl->setValue( v, EEmitPolicy::eEmitIfChanged, tmp != t );
    }
}

void CSpinBox64U::setValue( uint64_t value )
{
    fImpl->setValue( value, EEmitPolicy::eEmitIfChanged, true );
}

QString CSpinBox64U::prefix() const
{
    return fImpl->fPrefix;
}

void CSpinBox64U::setPrefix( const QString& prefix )
{
    fImpl->fPrefix = prefix;
    fImpl->updateEdit();

    fImpl->fCachedLongestAllowedString = QSize();
    fImpl->fCachedMinimumSizeHint = QSize(); // minimumSizeHint cares about the prefix
    updateGeometry();
}

QString CSpinBox64U::suffix() const
{
    return fImpl->fSuffix;
}

void CSpinBox64U::setSuffix( const QString& suffix )
{
    fImpl->fSuffix = suffix;
    fImpl->updateEdit();

    fImpl->fCachedLongestAllowedString = QSize();
    fImpl->fCachedMinimumSizeHint = QSize();
    updateGeometry();
}

QString CSpinBox64U::cleanText() const
{
    return fImpl->stripped( lineEdit()->displayText() );
}

uint64_t CSpinBox64U::singleStep() const
{
    return fImpl->fSingleStep;
}

void CSpinBox64U::setSingleStep( uint64_t value )
{
    if ( value >= 0 )
    {
        fImpl->fSingleStep = value;
        fImpl->updateEdit();
    }
}

void CSpinBox64U::stepBy( int steps )
{
    auto old = fImpl->fValue;
    EEmitPolicy e = EEmitPolicy::eEmitIfChanged;
    uint64_t singleStep = fImpl->fSingleStep;
    switch ( stepType() )
    {
        case QAbstractSpinBox::StepType::AdaptiveDecimalStepType:
            singleStep = fImpl->calculateAdaptiveDecimalStep( steps ).toULongLong();
            break;
        default:
            break;
    }
    
    double tmp = (double)fImpl->fValue + (double)( singleStep * steps );
    uint64_t tmpValue = 0;
    if ( tmp > maxAllowed() )
        tmpValue = maxAllowed();
    else if ( tmp < minAllowed() )
        tmpValue = minAllowed();
    else
        tmpValue = fImpl->fValue + ( singleStep * steps );

    fImpl->setValue( fImpl->bound( static_cast< uint64_t >( tmpValue ), old, steps ), e );
    selectAll();
}


void CSpinBox64U::connectLineEdit()
{
    if ( !lineEdit() )
        return;
    (void)connect( lineEdit(), &QLineEdit::textChanged, this, &CSpinBox64U::slotEditorTextChanged );
    (void)connect( lineEdit(), &QLineEdit::cursorPositionChanged, this, &CSpinBox64U::slotEditorCursorPositionChanged );
    (void)connect( lineEdit(), &QLineEdit::cursorPositionChanged, this, &CSpinBox64U::updateMicroFocus );
}

bool CSpinBox64U::wrapping() const
{
    return fImpl->fWrapping;
}

void CSpinBox64U::setWrapping( bool wrapping )
{
    fImpl->fWrapping = wrapping;
}

uint64_t CSpinBox64U::minimum() const
{
    return fImpl->fMinimum;
}

void CSpinBox64U::setMinimum( uint64_t minimum )
{
    setRange( minimum, ( fImpl->fMaximum > minimum ) ? fImpl->fMaximum : minimum );
}

uint64_t CSpinBox64U::maximum() const
{
    return fImpl->fMaximum;
}

void CSpinBox64U::setMaximum( uint64_t maximum )
{
    setRange( ( fImpl->fMinimum < maximum ) ? fImpl->fMinimum : maximum, maximum );
}

void CSpinBox64U::setRange( uint64_t min, uint64_t max )
{
    fImpl->setRange( min, max );
}

void CSpinBox64U::setStepType( QAbstractSpinBox::StepType stepType )
{
    fImpl->fStepType = stepType;
}

QAbstractSpinBox::StepType CSpinBox64U::stepType() const
{
    return fImpl->fStepType;
}

int CSpinBox64U::displayIntegerBase() const
{
    return fImpl->fDisplayBase;
}

void CSpinBox64U::setDisplayIntegerBase( int base )
{
    if ( Q_UNLIKELY( base < 2 || base > 36 ) )
    {
        qWarning( "CSpinBox64U::setDisplayIntegerBase: Invalid base (%d)", base );
        base = 10;
    }

    if ( base != fImpl->fDisplayBase )
    {
        fImpl->fDisplayBase = base;
        fImpl->updateEdit();
    }
}

QSize CSpinBox64U::sizeHint() const
{
    return fImpl->sizeHint();
}

QAbstractSpinBox::StepEnabled CSpinBox64U::stepEnabled() const
{
    return fImpl->stepEnabled();
}

QValidator::State CSpinBox64U::validate( QString& text, int& pos ) const
{
    QValidator::State state;
    fImpl->validateAndInterpret( text, pos, state );
    return state;
}


void CSpinBox64U::fixup( QString& input ) const
{
    if ( !isGroupSeparatorShown() )
        input.remove( locale().groupSeparator() );
}



