// The MIT License( MIT )
//
// Copyright( c ) 2020 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright noticeand this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef _NarcissisticNumbers_H
#define _NarcissisticNumbers_H

#include <QDialog>
#include <memory>
#include <unordered_map>
#include <functional>
#include <list>
namespace Ui {class CNarcissisticNumbers;};

class QStringListModel;
class QAbstractButton;

class CNarcissisticNumbers : public QDialog
{
    Q_OBJECT

public:
    CNarcissisticNumbers(QWidget *parent = 0);
    ~CNarcissisticNumbers();
    void keyPressEvent( QKeyEvent * event );

    int numValues() const;
    bool eventFilter(QObject *obj, QEvent *event);
    
public slots:
    void binaryOperatorClicked( char op );

    void btnEnterClicked();

    void btnCClicked();
    void btnDelClicked();
    void btnBSClicked();

    void btnAverageClicked();
    void btnNarcissisticClicked();
    void btnFactorsClicked( bool incNum );

    void reportPrime( std::list<int64_t>& factors, int64_t curr, bool incNum, int numShowsPrime );

    void btnPrimeFactorsClicked();
    void btnPerfectClicked();
    void btnSemiPerfectClicked();

    void btnWeirdClicked();
    void btnSublimeClicked();
    void btnAbundantClicked();
    void slotDataChanged();
private:
    template< typename T >
    T getLastValue( bool popLast );
    void addValue( char value );
    void addLastValue( double value );
    void addLastValue( bool value );
    void addLastValue( int64_t value );
    void addLastValue( const QString & newValue );
    std::pair< int64_t, std::list< int64_t > > getSumOfFactors( int64_t curr, bool properFactors ) const;
    std::pair< bool, std::list< int64_t > > isAbundant( int64_t num ) const;
    std::pair< bool, std::list< int64_t > > isSemiPerfect( int64_t num ) const;
    std::pair< bool, std::list< int64_t > > isPerfect( int64_t num ) const;

    std::list< int64_t > computeFactors( int64_t num ) const;
    std::list< int64_t > computePrimeFactors( int64_t num ) const;
    bool isSemiPerfect( const std::vector< int64_t >& numbers, size_t n, int64_t num ) const;

    std::unique_ptr< Ui::CNarcissisticNumbers > fImpl;

    void initMaps();
};

#endif // _ALCULATOR_H
