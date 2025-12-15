#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFileDialog>

//#define DEBUG_FILE

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);

    lexer = Lexer();
    parser = Parser(&lexer);

#ifdef DEBUG_FILE
    lexer.loadFile("/home/mainekun/dslcode/factorial.dsl");

    ui->runButton->click();
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_openFileButton_released()
{
    QString code_filename = QFileDialog::getOpenFileName(this,
                                 tr("Open File"),
                                 "$HOME",
                                 tr("DSL Files (*.dsl)"));

    lexer.loadFile(code_filename);

    QFile file(code_filename);
    file.open(QIODeviceBase::ReadOnly);

    ui->codeEdit->setText(file.readAll());

    file.close();

    ui->statusbar->showMessage("file loaded", 3000);
}


void MainWindow::on_useTextButton_released()
{
    QString code = ui->codeEdit->toPlainText();

    lexer.loadText(code);

    ui->statusbar->showMessage("Text loaded", 3000);
}


void MainWindow::on_runButton_released()
{
    try {
        if (lexer.analyze())
            ui->infoEdit->setText(tr("Tokenization succeeded!\nLexemas count: %1\n")
                .arg(lexer.get_tokenized_code().length()));

        ui->statusbar->showMessage(
            QString("Succeeded. Lexemas count : %1")
                .arg(lexer.get_tokenized_code().length()),
            10000
            );

        ui->tokenizedEdit->setText([&]()->QString{
            QString res = "";

            foreach(auto i, lexer.get_tokenized_code())
                res += i.toQString();

            return res;
        }());

        auto map = lexer.get_tables();
        auto tables = lexer.token_types;

        auto max = [](int a, int b) { return a > b ? a : b; };

        ui->tokenTable->setColumnCount(4);
        ui->tokenTable->setRowCount(max(
            max(map["words"].length(), map["ids"].length()),
            max(map["delimeters"].length(), map["consts"].length())));

        ui->tokenTable->setHorizontalHeaderLabels(tables);

        for (int table = 0; table < tables.size(); table++) {
            for (int i = 0; i < map[tables[table]].size(); i++)
                ui->tokenTable->setItem(i,table,
                    new QTableWidgetItem(map[tables[table]][i].value()));
        }

        if (parser.analyze())
            ui->infoEdit->append(tr("Parsing succeeded!\nConvolution sequance:"));

        ui->infoEdit->append([&]()->QString{
            QString r;
            foreach (auto& i, parser.conv_sequance())
                r += tr("%1: %2\n").arg(i.first).arg([&](){
                    QString buff;
                    foreach (auto lex, i.second) {
                        buff += (lex.value() == "a"
                                     ? lex.const_name() :
                                     lex.value())  + " ";
                    }

                    return buff;
                }());

            return r;
        }());

        if (parser.hasSemanticErrors())
            ui->infoEdit->append(
                parser.getSemanticErrors().join("\n"));
        else
            ui->infoEdit->append("Semantic analysis succceeded");

        AsmGenerator asmgen(&lexer);
        asmgen.generate(lexer.filename());

    }
    catch(std::exception& e) {
        ui->statusbar->showMessage(e.what(), 10000);
        ui->infoEdit->setText([&]()->QString{
            QString r;
            foreach (auto& i, parser.conv_sequance())
                r += tr("%1: %2\n").arg(i.first).arg([&](){
                    QString buff;
                    foreach (auto lex, i.second) {
                        buff += (lex.value() == "a"
                                     ? lex.const_name() :
                                     lex.value())  + " ";
                    }

                    return buff;
                }());

            return r += tr("\n") + tr(e.what());
        }());
    }

}

