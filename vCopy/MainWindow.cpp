#include "MainWindow.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QCloseEvent>
#include <QMessageBox>
#include <QRegExpValidator>
#include "Copier.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);
	progressBar->hide();
	extensionLineEdit->setValidator(new QRegExpValidator(QRegExp("[A-Za-z0-9]{0,8}"), this));
	connect(sourceButton, &QPushButton::clicked, this, &MainWindow::selectSourceDir);
	connect(fileNameButton, &QPushButton::clicked, this, &MainWindow::selectFileNamesFile);
	connect(destinationButton, &QPushButton::clicked, this, &MainWindow::selectDestinationDir);
	connect(saveLogsButton, &QPushButton::clicked, this, &MainWindow::saveLogs);
	connect(extensionCheckBox, &QCheckBox::stateChanged, this, &MainWindow::extCheckBoxStateChanged);
	connect(okButton, &QPushButton::clicked, this, &MainWindow::perform);
	connect(stopButton, &QPushButton::clicked, this, &MainWindow::stop);
	connect(cancelButton, &QPushButton::clicked, this, &MainWindow::close);
}

void MainWindow::selectSourceDir()
{
	QString caption = QString::fromStdWString(L"Chọn thư mục nguồn");
	QString dir = QFileDialog::getExistingDirectory(this, caption);
	if (!dir.isEmpty())
	{
		sourceLineEdit->setText(dir);
	}
}

void MainWindow::started()
{
	progressBar->show();
	progressBar->setValue(0);
}

void MainWindow::stoped()
{
	QApplication::beep();
	progressBar->hide();
	textEdit->append(QString::fromStdWString(L" [i] Đã dừng"));
	mIsRunning = false;
	stopButton->setEnabled(false);
}

void MainWindow::finished()
{
	QApplication::beep();
	progressBar->hide();
	textEdit->append(QString::fromStdWString(L"--- Hoàn thành ---"));
	mIsRunning = false;
	stopButton->setEnabled(false);
}

void MainWindow::report(int quantity, int total)
{
	textEdit->append("---");
	textEdit->append(QString::fromStdWString(L" [i] Đã thực hiện:   %1/%2").arg(quantity).arg(total));
	textEdit->append(QString::fromStdWString(L" [i] Chưa thực hiện: %1/%2").arg(total-quantity).arg(total));
}

void MainWindow::notice(const QString& message)
{
	textEdit->append(message);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	if (mIsRunning)
	{
		QMessageBox::StandardButton answer = QMessageBox::question(this, QString::fromStdWString(L"Thoát"),
			QString::fromStdWString(L"Quá trình thực hiện chưa hoàn thành. Bạn vẫn muốn thoát chứ?"));
		if (answer == QMessageBox::Yes)
		{
			emit stopPerform();
			event->accept();
		}
		else
		{
			event->ignore();
		}
	}
	else
	{
		event->accept();
	}
}

void MainWindow::selectFileNamesFile()
{
	QString caption = QString::fromStdWString(L"Chọn danh sách tệp tin");
	QString fileName = QFileDialog::getOpenFileName(this, caption, QString(), "Plain Text (*.txt)");
	if (!fileName.isEmpty())
	{
		fileNameLineEdit->setText(fileName);
	}
}

void MainWindow::selectDestinationDir()
{
	QString caption = QString::fromStdWString(L"Chọn thư mục đích");
	QString dir = QFileDialog::getExistingDirectory(this, caption);
	if (!dir.isEmpty())
	{
		destinationLineEdit->setText(dir);
	}
}

void MainWindow::extCheckBoxStateChanged(int state)
{
	extensionLineEdit->setEnabled(state == Qt::Checked);
}

void MainWindow::saveLogs()
{
	QString caption = QString::fromStdWString(L"Lưu logs file");
	QString fileName = QFileDialog::getSaveFileName(this, caption, QString(), "Plain Text (*.txt)");
	if (!fileName.isEmpty())
	{
		QFile file(fileName);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream textStream(&file);
			textStream.setCodec("UTF-8");
			textStream << textEdit->toPlainText();
			file.close();
		}
	}
}

void MainWindow::perform()
{
	QString sourceDir = sourceLineEdit->text().trimmed();
	QString destinationDir = destinationLineEdit->text().trimmed();
	QString fileNamesFile = fileNameLineEdit->text().trimmed();
	QString extension = extensionCheckBox->isChecked() ? extensionLineEdit->text() : QString();
	bool copy = copyRadioButton->isChecked();

	textEdit->clear();
	if (sourceDir.isEmpty() || destinationDir.isEmpty() || fileNamesFile.isEmpty())
	{
		QApplication::beep();
		textEdit->append(QString::fromStdWString(L" [i] Chưa nhập đủ thông tin."));
		QTimer::singleShot(5000, [this] { textEdit->clear();  });
		return;
	}

	QStringList fileNames = readFileNames(fileNamesFile);
	if (fileNames.empty())
	{
		QApplication::beep();
		textEdit->append(QString::fromStdWString(L" [i] Không thể đọc danh sách tệp tin."));
		QTimer::singleShot(5000, [this] { textEdit->clear();  });
		return;
	}

	if (extensionCheckBox->isChecked() && extension.isEmpty())
	{
		QApplication::beep();
		textEdit->append(QString::fromStdWString(L" [i] Chưa nhập định dạng của tệp tin."));
		QTimer::singleShot(5000, [this] { textEdit->clear();  });
		return;
	}

	QThread* thread = new QThread;
	Copier* copier = new Copier(sourceDir, destinationDir, fileNames, extension, copy);
	copier->moveToThread(thread);

	connect(copier, SIGNAL(started()), this, SLOT(started()));
	connect(copier, SIGNAL(stoped()), this, SLOT(stoped()));
	connect(copier, SIGNAL(finished()), this, SLOT(finished()));
	connect(copier, SIGNAL(notice(const QString&)), this, SLOT(notice(const QString&)));
	connect(copier, SIGNAL(progress(int)), progressBar, SLOT(setValue(int)));
	connect(copier, SIGNAL(report(int, int)), this, SLOT(report(int, int)));

	connect(copier, SIGNAL(finished()), thread, SLOT(quit()));
	connect(copier, SIGNAL(finished()), copier, SLOT(deleteLater()));

	connect(thread, SIGNAL(started()), this, SLOT(preparing()));
	connect(thread, SIGNAL(started()), copier, SLOT(run()));
	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

	//Khi this phát tín hiệu stoped thì scanner thực hiện phương thức stop
	connect(this, SIGNAL(stopPerform()), copier, SLOT(stop()), Qt::DirectConnection);
	thread->start();
}

void MainWindow::stop()
{
	if (mIsRunning)
	{
		textEdit->append(QString::fromStdWString(L" [i] Lệnh dừng đang được thực hiện..."));
		stopButton->setEnabled(false);
		emit stopPerform();
	}
}

void MainWindow::preparing()
{
	textEdit->clear();
	mIsRunning = true;
	stopButton->setEnabled(true);
}

QStringList MainWindow::readFileNames(const QString& fileNamesFile)
{
	QStringList fileNames;
	QString plainText = fileNameLineEdit->text().trimmed();
	if (plainText.isEmpty())
		return fileNames;

	QFile file(plainText);
	if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream textStream(&file);
		textStream.setCodec("UTF-8");
		while (!textStream.atEnd())
		{
			QString fileName = textStream.readLine().trimmed();
			if (!fileName.isEmpty())
			{
				fileNames.append(fileName);
			}
		}
		file.close();
	}
	return fileNames;
}
