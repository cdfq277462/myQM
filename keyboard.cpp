#include "keyboard.h"
#include <QtCore>
#include <QtDebug>
#include <QPushButton>
#include <QGridLayout>
#include <QLineEdit>
#include <QSignalMapper>
#include <QDesktopWidget>
#include <QDebug>
#include <QKeyEvent>
#include <QApplication>
#include <QCoreApplication>


keyboard::keyboard(QWidget *parent)
    : QDialog(parent)
{
    initWidgets();
}
void keyboard::initWidgets()
{
    QGridLayout *gridLayout = new QGridLayout(this);
    QSignalMapper *signalMapper = new QSignalMapper(this);

    this->setCursor(Qt::BlankCursor);   //hide mouse

    for (int i = 0; i < 10; i++)
    {
        QString text;
        text = QString::number(i);
        numBt[i] = new QPushButton(this);
        numBt[i]->setText(text);

        connect(numBt[i], SIGNAL(clicked()), signalMapper, SLOT(map()));
        signalMapper->setMapping(numBt[i], i);

    }

    dotBt = new QPushButton(this);
    dotBt->setText(".");


    //minusBt = new QPushButton(this);
    //minusBt->setText("-");

    backspaceBt = new QPushButton(this);
    backspaceBt->setObjectName("Backspace");
    backspaceBt->setText("Bkspace");

    enterBt = new QPushButton(this);
    enterBt->setObjectName("Enter");
    enterBt->setText("Enter");

    connect(dotBt, SIGNAL(clicked()), signalMapper, SLOT(map()));
    //connect(minusBt, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(backspaceBt, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(enterBt, SIGNAL(clicked()), signalMapper, SLOT(map()));

    signalMapper->setMapping(dotBt, 10);
    //signalMapper->setMapping(minusBt, 11);
    signalMapper->setMapping(backspaceBt, 12);
    signalMapper->setMapping(enterBt, 13);

    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(clickButton(int)));

    gridLayout->setSpacing(0);
    gridLayout->setSizeConstraint(QLayout::SetFixedSize);

    gridLayout->addWidget(backspaceBt, 1, 3, 1, 1, Qt::AlignCenter);
    gridLayout->addWidget(enterBt, 2, 3, 4, 1, Qt::AlignCenter);

    gridLayout->addWidget(numBt[7], 1, 0, 1, 1, Qt::AlignCenter);
    gridLayout->addWidget(numBt[8], 1, 1, 1, 1, Qt::AlignCenter);
    gridLayout->addWidget(numBt[9], 1, 2, 1, 1, Qt::AlignCenter);

    gridLayout->addWidget(numBt[4], 2, 0, 1, 1, Qt::AlignCenter);
    gridLayout->addWidget(numBt[5], 2, 1, 1, 1, Qt::AlignCenter);
    gridLayout->addWidget(numBt[6], 2, 2, 1, 1, Qt::AlignCenter);

    gridLayout->addWidget(numBt[1], 3, 0, 1, 1, Qt::AlignCenter);
    gridLayout->addWidget(numBt[2], 3, 1, 1, 1, Qt::AlignCenter);
    gridLayout->addWidget(numBt[3], 3, 2, 1, 1, Qt::AlignCenter);

    gridLayout->addWidget(numBt[0], 4, 1, 1, 1, Qt::AlignCenter);
    gridLayout->addWidget(dotBt,    4, 2, 1, 1, Qt::AlignCenter);
    //gridLayout->addWidget(minusBt,  4, 2, 1, 1, Qt::AlignCenter);

    //set style
    QFile file(":/keyboard.css");

    if (file.open(QFile::ReadOnly))
    {
        QString css = QLatin1String(file.readAll());
        QWidget::setStyleSheet(css);
    }
    file.close();

}


void keyboard::clickButton(int index)
{
    QKeyEvent *keyPressEvent = NULL;

    if (index >=0 && index < 10)
    {
        /* 0-9 */
        QString text;
        text = QString::number(index);
        keyPressEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_0 + index,
                                      Qt::NoModifier, text);
    }
    else if (index == 10)
    {
        /* dot */
        keyPressEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Period,
                                      Qt::NoModifier, ".");
    }
    else if (index == 11)
    {
        /* minus */
        keyPressEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Minus,
                                      Qt::NoModifier, "-");
    }
    else if (index == 12)
    {
        /* backspace */
        keyPressEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Backspace,
                                      Qt::NoModifier);
    }
    else if (index == 13)
    {
        /* enter */
        //emit commit(str);
        hide();
    }

    if (keyPressEvent != NULL)
    {
        emit KeyboardSig(keyPressEvent);
        //QGuiApplication::postEvent(inputEdit, keyPressEvent);
    }

}
void keyboard::showKeyboard(QPoint pt, QRect focusWidget)
{
    //mainwiindows = 800*480
    QWidget::show();

    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect keyboardRect = QWidget::rect();
    QRect screenRect = desktopWidget->screenGeometry();

    //qDebug() << "dialog show" << endl;
    //qDebug() << pt.x() << " " << pt.y() << endl;


    QSize keyboardSize = QWidget::size();

    int marginY = pt.y() + focusWidget.height() + keyboardSize.height();
    int marginX = pt.x() + focusWidget.width() + keyboardSize.width();
    if(marginY > 480)
    {
        pt.setY(pt.y() - (marginY - 480));

        if(marginX > 800)
            pt.setX(pt.x() - keyboardSize.width());
        else
            pt.setX(pt.x() + focusWidget.width());
    }

    else if(marginX > 800)
        pt.setX(pt.x() - keyboardSize.width());

    else
        pt.setX(pt.x() + focusWidget.width());


    QWidget::move(pt);
}

void keyboard::hideKeyboard()
{
    QWidget::hide();
}
bool keyboard::isVisible() const
{
    return QWidget::isVisible();
}
void keyboard::pressKey(int key)
{
    //unused
}
