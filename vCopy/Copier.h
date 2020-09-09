#ifndef COPIER_H
#define COPIER_H

#include <QObject>
#include <QMutex>

class Copier : public QObject
{
	Q_OBJECT

public:
	Copier(const QString& sourceDir, const QString& destinationDir, const QStringList& fileNames,
		const QString& extension = QString(), bool copy = true);
	~Copier() = default;

private slots:
	void stop();
	void run();

signals:
	void started();
	void stoped();
	void finished();
	void report(int quantity, int total);
	void progress(int value);
	void notice(const QString& message);

private:
	QMutex mMutex;
	bool mCopy;
	bool mStop;
	QStringList mFileNames;
	QString mExtension;
	QString mSourceDir;
	QString mDestinationDir;
};

#endif