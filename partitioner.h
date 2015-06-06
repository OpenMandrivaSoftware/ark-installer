#ifndef _PARTITIONER_H_
#define _PARTITIONER_H_ 1
#include <qframe.h>
#include <qstringlist.h>
#include <qmap.h>
#include <qlabel.h>
#include <qprogressbar.h>
#include "partedtimer.h"

struct FileSystem {
	FileSystem(QString const &mp=QString::null, QString const &fs="ext3"):mountpoint(mp),filesystem(fs) { }
	QString mountpoint;
	QString filesystem;
};

class PartitionMap:public QMap<QString,FileSystem> {
public:
	PartitionMap():QMap<QString,FileSystem>() { }
	bool hasMountpoint(QString const &mountpoint) const {
		for(ConstIterator it=begin(); it!=end(); ++it)
			if(it.value().mountpoint==mountpoint)
				return true;
		return false;
	}
	QString device(QString const &mountpoint) const {
		for(ConstIterator it=begin(); it!=end(); ++it)
			if(it.value().mountpoint==mountpoint)
				return it.key();
		return QString::null;
	}
};

class Partitioner:public QFrame
{
	Q_OBJECT
public:
	Partitioner(QWidget *parent, Qt::WindowFlags f=0);
	virtual ~Partitioner();
	virtual void init()=0; // Stuff that should be in the constructor - unfortunately, a constructor can't emit signals
	enum GrubLocation { MBR, Partition, Nowhere };
	PartitionMap partitions() const { return _partitions; }
	unsigned long size(QString const &part) const { return _size[part]; }
	QString	rootPartition() { return _partitions.device("/"); }
	QString	bootPartition() const { return _partitions.device("/boot"); }
	void setHelp(QString const &h);
	GrubLocation const &grubLocation() const { return _grubLocation; }
protected slots:
	virtual void updateProgress(PedTimer *t);
	virtual void abort();
signals:
	void done();
protected:
	QLabel				*help;
	QProgressBar			*progress;
	PartedTimer			*timer;
	unsigned long long		_totalSize;
	unsigned long long		_dosSize;
	QString				_swap;
	QString				_rootfs;
	QString				_mkfsopts;
	QString				_postmkfs;
	PartitionMap			_partitions;
	QMap<QString,unsigned long>	_size;
	GrubLocation			_grubLocation;
};
#endif
