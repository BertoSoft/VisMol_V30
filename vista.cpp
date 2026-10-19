#include "vista.h"
#include "ui_vista.h"

Vista::Vista(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Vista)
{
    ui->setupUi(this);
}

Vista::~Vista()
{
    delete ui;
}
