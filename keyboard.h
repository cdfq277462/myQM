#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <QDialog>

class QGridLayout;
class QLineEdit;
class QPushButton;
class QRegExpValidator;
class QDBusVariant;


class keyboard : public QDialog
{
    Q_OBJECT
public:
    keyboard(QWidget *parent = nullptr);

    void initWidgets();

signals:
    void KeyboardSig(QKeyEvent*);

public slots:
    void clickButton(int index);
    void showKeyboard(QPoint pt, QRect focusWidget);
    void hideKeyboard();
    bool isVisible() const;
    void pressKey(int key);


private:
    //bool eventFilter(QObject *, QEvent *);

    QPushButton *numBt[10];
    QPushButton *dotBt;
    QPushButton *minusBt;
    QPushButton *backspaceBt;
    QPushButton *enterBt;
    //QLineEdit *inputEdit;

};

#endif // KEYBOARD_H
