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

#ifndef _NARCISSISTICNUMBERS_H
#define _NARCISSISTICNUMBERS_H

#include <QDialog>
#include <memory>
#include <unordered_map>
#include <functional>
#include <list>
#include <chrono>

namespace Ui {class CNarcissisticNumbers;};
class CNarcissisticNumCalculator;
class QStringListModel;
class QAbstractButton;
class QTimer;

class CNarcissisticNumbers : public QDialog
{
    Q_OBJECT

public:
    CNarcissisticNumbers(QWidget *parent = 0);

    ~CNarcissisticNumbers();

public slots:
    void slotChanged();
    void slotRun();
    void slotShowResults();

    void updateUI( bool finished );

private:
    void setNumbersList( const std::list< int64_t >& numbers );
    std::list< int64_t > getNumbersList() const;

    void loadSettings();
    void saveSettings() const;

    QString getNumberList( const std::list<int64_t> & numbers ) const;

    QTimer * fMonitorTimer{nullptr};
    int fFinishedCount{ 0 };
    std::unique_ptr< Ui::CNarcissisticNumbers > fImpl;
    std::unique_ptr< CNarcissisticNumCalculator > fCalculator;
};

#endif 
