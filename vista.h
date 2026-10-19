#ifndef VISTA_H
#define VISTA_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class Vista;
}
QT_END_NAMESPACE

class Vista : public QMainWindow
{
    Q_OBJECT

public:
    explicit Vista(QWidget *parent = nullptr);
    ~Vista() override;

private:
    Ui::Vista *ui;
};
#endif // VISTA_H
