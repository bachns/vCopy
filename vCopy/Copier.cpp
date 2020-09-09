#include "Copier.h"
#include <QDirIterator>

Copier::Copier(const QString& sourceDir, const QString& destinationDir,
	const QStringList& fileNames, const QString& extension, bool copy)
	: mSourceDir(sourceDir), mDestinationDir(destinationDir), mFileNames(fileNames), mExtension(extension)
	, mCopy(copy), mStop(false)
{
}

void Copier::stop()
{
	QMutexLocker locker(&mMutex);
	mStop = true;
}

void Copier::run()
{
	emit started();
	const QString errorString = QString::fromStdWString(L"<font color='#ff007f'>[Lỗi] </font>");
	const QString okString = QString::fromStdWString(L"<font color='#55aa00'>[Xong] </font>");
	int quantity = 0;
	for (int i = 0; !mStop && i < mFileNames.size(); ++i)
	{
		emit progress((i + 1) * 100 / mFileNames.size());
		QString name = mFileNames.at(i) + QString(".") + mExtension;
		QString src = mSourceDir + QDir::separator() + name;
		if (QFile::exists(src))
		{
			QString dst = mDestinationDir + QDir::separator() + name;
			QFile::remove(dst);
			if (QFile::copy(src, dst))
			{
				emit notice(okString + name);
				quantity++;
			}
			else
			{
				emit notice(errorString + name);
			}

			if (!mCopy)
			{
				QFile::remove(src);
			}
		}
		else
		{
			emit notice(errorString + name);
		}

		QMutexLocker locker(&mMutex);
		if (mStop)
			break;
	}
	emit report(quantity, mFileNames.size());
	mStop ? emit stoped() : emit finished();
}