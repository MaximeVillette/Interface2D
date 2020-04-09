#ifndef SendFrameBox_H
#define SendFrameBox_H

#include <QCanBusFrame>
#include <QGroupBox>
#include <QRegularExpression>
#include <QValidator>

QT_BEGIN_NAMESPACE
namespace Ui {
class MessagesEnvoyes;
}
QT_END_NAMESPACE

class HexIntegerValidator : public QValidator
{
    Q_OBJECT
public:
    explicit HexIntegerValidator(QObject *parent = nullptr);

    QValidator::State validate(QString &input, int &) const;

    void setMaximum(uint maximum);

private:
    uint m_maximum = 0;
};

class HexStringValidator : public QValidator
{
    Q_OBJECT

public:
    explicit HexStringValidator(QObject *parent = nullptr);

    QValidator::State validate(QString &input, int &pos) const;

    void setMaxLength(int maxLength);

private:
    int m_maxLength = 0;
};

class SendFrameBox : public QGroupBox
{
    Q_OBJECT

public:
    explicit SendFrameBox(QWidget *parent = nullptr);
    ~SendFrameBox();

signals:
    void sendFrame(const QCanBusFrame &frame);

private:
    Ui::MessagesEnvoyes *m_ui = nullptr;

    HexIntegerValidator *m_hexIntegerValidator = nullptr;
    HexStringValidator *m_hexStringValidator = nullptr;

    //void keyPressEvent(QKeyEvent *keyevent);
};

#endif // SendFrameBox_H
