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

#include "SpinBox64.h"

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

class CSpinBox64Impl
{
public:
    CSpinBox64Impl( CSpinBox64 * parent ) : fParent( parent ){};

    virtual int64_t valueFromText( const QString& text ) const;
    virtual QString textFromValue( int64_t val ) const;
    void setValue( int64_t value, EEmitPolicy ep, bool update = true );
    QVariant validateAndInterpret( QString& input, int& pos, QValidator::State& state ) const;
    QVariant calculateAdaptiveDecimalStep( int steps ) const;
    void updateEdit();
    QString stripped( const QString& text, int* pos = 0 ) const;
    void clearCache() const;
    int64_t bound( const int64_t& val, const int64_t& old = 0, int steps = 0 ) const;
    bool specialValue() const;
    void emitSignals( EEmitPolicy ep, const int64_t& old );
    void setRange( int64_t min, int64_t max );
    QSize sizeHint() const;
    QAbstractSpinBox::StepEnabled stepEnabled() const;
    QString longestAllowedString() const;

    int64_t fValue{ 0 };
    int64_t fMinimum{ 0 };
    int64_t fMaximum{ 99 };
    int64_t fSingleStep{ 1 };
    int fDisplayBase{ 10 };
    QString fPrefix;
    QString fSuffix;
    mutable QSize fCachedSizeHint;
    mutable QSize fCachedMinimumSizeHint;
    QString fSpecialValueText;

    mutable QString fCachedText;
    mutable int64_t fCachedValue;
    mutable QValidator::State fCachedState;
    mutable QString fCachedLongestAllowedString;

    bool fWrapping{ false };
    QAbstractSpinBox::StepType fStepType{ QAbstractSpinBox::StepType::DefaultStepType };
    bool fIgnoreCursorPositionChanged{ false };

    CSpinBox64 * fParent{ nullptr };
};

void CSpinBox64Impl::setValue( int64_t value, EEmitPolicy ep, bool update )
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

void CSpinBox64Impl::emitSignals( EEmitPolicy ep, const int64_t& old )
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

QString CSpinBox64Impl::stripped( const QString& t, int* pos ) const
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

void CSpinBox64Impl::clearCache() const
{
    fCachedText.clear();
    fCachedValue = 0;
    fCachedLongestAllowedString = QString();
    fCachedState = QValidator::Acceptable;
}

int64_t CSpinBox64Impl::bound( const int64_t& val, const int64_t& old, int steps ) const
{
    int64_t v = val;
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

bool CSpinBox64Impl::specialValue() const
{
    return ( fValue == fMinimum && !fSpecialValueText.isEmpty() );
}

int64_t CSpinBox64Impl::valueFromText( const QString& text ) const
{
    QString copy = text;
    int pos = fParent->lineEdit()->cursorPosition();
    QValidator::State state = QValidator::Acceptable;
    return validateAndInterpret( copy, pos, state ).toLongLong();
}

QAbstractSpinBox::StepEnabled CSpinBox64Impl::stepEnabled() const
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

QVariant CSpinBox64Impl::validateAndInterpret( QString& input, int& pos, QValidator::State& state ) const
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
            num = copy.toLongLong( &ok, fDisplayBase );
        }
        else
        {
            num = fParent->locale().toLongLong( copy, &ok );
            if ( !ok && ( fMaximum >= 1000 || fMinimum <= -1000 ) )
            {
                const QChar sep = fParent->locale().groupSeparator();
                const QChar doubleSep[ 2 ] = { sep, sep };
                if ( copy.contains( sep ) && !copy.contains( QString( doubleSep, 2 ) ) )
                {
                    QString copy2 = copy;
                    copy2.remove( fParent->locale().groupSeparator() );
                    num = fParent->locale().toLongLong( copy2, &ok );
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

QVariant CSpinBox64Impl::calculateAdaptiveDecimalStep( int steps ) const
{
    const int64_t intValue = fValue;
    const int64_t absValue = std::abs( intValue );

    if ( absValue < 100 )
        return 1;

    const bool valueNegative = intValue < 0;
    const bool stepsNegative = steps < 0;
    const int signCompensation = ( valueNegative == stepsNegative ) ? 0 : 1;

    const int log = static_cast<int>( std::log10( absValue - signCompensation ) ) - 1;
    return static_cast<int64_t>( std::pow( 10, log ) );
}

void CSpinBox64Impl::updateEdit()
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

void CSpinBox64Impl::setRange( int64_t min, int64_t max )
{
    clearCache();
    fMinimum = min;
    fMaximum = ( min < max ) ? max : min;
    fCachedSizeHint = QSize();
    fCachedMinimumSizeHint = QSize(); // minimumSizeHint cares about min/max
    fCachedLongestAllowedString = QString();

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

QString CSpinBox64Impl::longestAllowedString() const
{
    if ( fCachedLongestAllowedString.isEmpty() )
    {
        QString minStr = textFromValue( fMinimum );
        QString maxStr = textFromValue( fMaximum );
        const QFontMetrics fm( fParent->fontMetrics() );
        if ( fm.horizontalAdvance( minStr ) > fm.horizontalAdvance( maxStr ) )
            fCachedLongestAllowedString = minStr;
        else
            fCachedLongestAllowedString = maxStr;
    }
    return fCachedLongestAllowedString;
}

QSize CSpinBox64Impl::sizeHint() const
{
    if ( fCachedSizeHint.isEmpty() )
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
        fCachedSizeHint = fParent->style()->sizeFromContents( QStyle::CT_SpinBox, &opt, hint, fParent ).expandedTo( QApplication::globalStrut() );
    }
    return fCachedSizeHint;
}

QString CSpinBox64Impl::textFromValue( int64_t value ) const
{
    QString str;

    if ( fDisplayBase != 10 )
    {
        const QLatin1String prefix = value < 0 ? QLatin1String( "-" ) : QLatin1String();
        str = prefix + QString::number( std::abs( value ), fDisplayBase );
    }
    else
    {
        str = fParent->locale().toString( value );
        if ( !fParent->isGroupSeparatorShown() && ( std::abs( value ) >= 1000 || value == INT_MIN ) )
        {
            str.remove( fParent->locale().groupSeparator() );
        }
    }

    return str;
}

Q_DECLARE_METATYPE( int64_t );

CSpinBox64::CSpinBox64( QWidget* parent /*= 0 */ ) :
    QAbstractSpinBox( parent ),
    fImpl( new CSpinBox64Impl( this ) )
{
    qRegisterMetaType< int64_t >( "int64_t" );
    connectLineEdit();
}

CSpinBox64::~CSpinBox64()
{
}

int64_t CSpinBox64::value() const
{
    return fImpl->fValue;
}

void CSpinBox64::slotEditorCursorPositionChanged( int oldpos, int newpos )
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

void CSpinBox64::slotEditorTextChanged( const QString& t )
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

void CSpinBox64::setValue( int64_t value )
{
    fImpl->setValue( value, EEmitPolicy::eEmitIfChanged, true );
}

QString CSpinBox64::prefix() const
{
    return fImpl->fPrefix;
}

void CSpinBox64::setPrefix( const QString& prefix )
{
    fImpl->fPrefix = prefix;
    fImpl->updateEdit();

    fImpl->fCachedSizeHint = QSize();
    fImpl->fCachedMinimumSizeHint = QSize(); // minimumSizeHint cares about the prefix
    updateGeometry();
}

QString CSpinBox64::suffix() const
{
    return fImpl->fSuffix;
}

void CSpinBox64::setSuffix( const QString& suffix )
{
    fImpl->fSuffix = suffix;
    fImpl->updateEdit();

    fImpl->fCachedSizeHint = QSize();
    fImpl->fCachedMinimumSizeHint = QSize();
    updateGeometry();
}

QString CSpinBox64::cleanText() const
{
    return fImpl->stripped( lineEdit()->displayText() );
}

int64_t CSpinBox64::singleStep() const
{
    return fImpl->fSingleStep;
}

void CSpinBox64::setSingleStep( int64_t value )
{
    if ( value >= 0 )
    {
        fImpl->fSingleStep = value;
        fImpl->updateEdit();
    }
}

void CSpinBox64::stepBy( int steps )
{
    auto old = fImpl->fValue;
    EEmitPolicy e = EEmitPolicy::eEmitIfChanged;
    int64_t singleStep = fImpl->fSingleStep;
    switch ( stepType() )
    {
        case QAbstractSpinBox::StepType::AdaptiveDecimalStepType:
            singleStep = fImpl->calculateAdaptiveDecimalStep( steps ).toLongLong();
            break;
        default:
            break;
    }
    
    double tmp = (double)fImpl->fValue + (double)( singleStep * steps );
    int64_t tmpValue = 0;
    if ( tmp > maxAllowed() )
        tmpValue = maxAllowed();
    else if ( tmp < minAllowed() )
        tmpValue = minAllowed();
    else
        tmpValue = fImpl->fValue + ( singleStep * steps );

    fImpl->setValue( fImpl->bound( static_cast< int64_t >( tmpValue ), old, steps ), e );
    selectAll();
}


void CSpinBox64::connectLineEdit()
{
    if ( !lineEdit() )
        return;
    (void)connect( lineEdit(), &QLineEdit::textChanged, this, &CSpinBox64::slotEditorTextChanged );
    (void)connect( lineEdit(), &QLineEdit::cursorPositionChanged, this, &CSpinBox64::slotEditorCursorPositionChanged );
    (void)connect( lineEdit(), &QLineEdit::cursorPositionChanged, this, &CSpinBox64::updateMicroFocus );
}

bool CSpinBox64::wrapping() const
{
    return fImpl->fWrapping;
}

void CSpinBox64::setWrapping( bool wrapping )
{
    fImpl->fWrapping = wrapping;
}

int64_t CSpinBox64::minimum() const
{
    return fImpl->fMinimum;
}

void CSpinBox64::setMinimum( int64_t minimum )
{
    setRange( minimum, ( fImpl->fMaximum > minimum ) ? fImpl->fMaximum : minimum );
}

int64_t CSpinBox64::maximum() const
{
    return fImpl->fMaximum;
}

void CSpinBox64::setMaximum( int64_t maximum )
{
    setRange( ( fImpl->fMinimum < maximum ) ? fImpl->fMinimum : maximum, maximum );
}

void CSpinBox64::setRange( int64_t min, int64_t max )
{
    fImpl->setRange( min, max );
}

void CSpinBox64::setStepType( QAbstractSpinBox::StepType stepType )
{
    fImpl->fStepType = stepType;
}

QAbstractSpinBox::StepType CSpinBox64::stepType() const
{
    return fImpl->fStepType;
}

int CSpinBox64::displayIntegerBase() const
{
    return fImpl->fDisplayBase;
}

void CSpinBox64::setDisplayIntegerBase( int base )
{
    if ( Q_UNLIKELY( base < 2 || base > 36 ) )
    {
        qWarning( "CSpinBox64::setDisplayIntegerBase: Invalid base (%d)", base );
        base = 10;
    }

    if ( base != fImpl->fDisplayBase )
    {
        fImpl->fDisplayBase = base;
        fImpl->updateEdit();
    }
}

QSize CSpinBox64::sizeHint() const
{
    return fImpl->sizeHint();
}

QAbstractSpinBox::StepEnabled CSpinBox64::stepEnabled() const
{
    return fImpl->stepEnabled();
}

QValidator::State CSpinBox64::validate( QString& text, int& pos ) const
{
    QValidator::State state;
    fImpl->validateAndInterpret( text, pos, state );
    return state;
}


void CSpinBox64::fixup( QString& input ) const
{
    if ( !isGroupSeparatorShown() )
        input.remove( locale().groupSeparator() );
}



