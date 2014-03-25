#include <QFile>
#include <QPainter>
#include "board.h"
#include "base.h"
//-------------------KBase------------------------

KBase::KBase(int nInNum, int nOutNum, 
	const QString& name, 
	const QString& description, 
	const QPainterPath& path /*= QPainterPath()*/,
	const QList<IPinPos>& pinPosList /*= QList<IPinPos>()*/)
	: m_nInPinNum(nInNum)
	, m_nOutPinNum(nOutNum)
	, m_sName(name)
	, m_sDescription(description)
	, m_path(path)
	, m_pinPosList(pinPosList)
	, m_pBoard(0)
{
	m_width = m_path.boundingRect().width();
	m_height =m_path.boundingRect().height();

	m_nPinNum = m_nInPinNum + m_nOutPinNum;
	m_pPinLevelList = new LevelSignal[m_nPinNum];
	//将电平信号默认值设置为-1，表示为通电
	for (int i = 0; i < m_nPinNum; ++i)
		m_pPinLevelList[i] = NOSIGNAL;
}

KBase::KBase(const KBase& other)
	: m_nInPinNum(other.m_nInPinNum)
	, m_nOutPinNum(other.m_nOutPinNum)
	, m_sName(other.m_sName)
	, m_sDescription(other.m_sDescription)
	, m_pPinLevelList(NULL)
	, m_path(other.m_path)
	, m_pinPosList(other.m_pinPosList)
	, m_width(other.m_width)
	, m_height(other.m_height)
{
	m_nPinNum = m_nInPinNum + m_nOutPinNum;
	if (other.m_pPinLevelList)
	{
		m_pPinLevelList = new LevelSignal[m_nPinNum + 1];
		for (int i = 0; i < m_nPinNum; ++i)
			m_pPinLevelList[i] = other.m_pPinLevelList[i];
	}
}

KBase::~KBase()
{
	if (m_pPinLevelList)
		delete [] m_pPinLevelList;
}

int KBase::getInPinNum() const 
{
	return m_nInPinNum;
}

int KBase::getOutPinNum() const 
{
	return m_nOutPinNum;
}

int KBase::getPinNum() const 
{
	return m_nPinNum;
}

const QList<ILink>& KBase::getLinks() const 
{
	return m_links;
}

const QString& KBase::getName() const
{
	return m_sName;
}

const QString& KBase::getDescription() const
{
	return m_sDescription;
}

void KBase::reset(LevelSignal val /* = NOSIGNAL */)
{
	for (int i = 0; i < m_nPinNum; ++i)
		m_pPinLevelList[i] = val;
}

LevelSignal KBase::get(int num /* = 0 */) const
{
	if (!m_pPinLevelList || num < 0 || num >= m_nPinNum)
		assert(false);
	return m_pPinLevelList[num];
}

LevelSignal KBase::getOut(int num /* = 0 */) const
{
	if (!m_pPinLevelList || num < 0 || num >= m_nOutPinNum)
		assert(false);
	return m_pPinLevelList[num + m_nInPinNum];
}

LevelSignal KBase::getIn(int num /* = 0*/) const
{
	if (!m_pPinLevelList || num < 0 || num >= m_nInPinNum)
		assert(false);
	return m_pPinLevelList[num];
}

void KBase::setIn(int num, LevelSignal val)
{
	if (!m_pPinLevelList || num < 0 || num >= m_nInPinNum)
	{	
		assert(!"给定下标超出，或m_pPinLevelList == NULL");
		return;
	}
	if (m_pPinLevelList[num] != val)
	{
		LevelSignal* pOutList = new LevelSignal[m_nOutPinNum];
		for (int i = 0; i < m_nOutPinNum; ++i)//保存计算前输出电平
			pOutList[i] = m_pPinLevelList[i + m_nInPinNum];

		m_pPinLevelList[num] = val;
		calculate();

		for (int i = 0; i < m_nOutPinNum; ++i)//发送输出电平变化信息
			if (pOutList[i] != m_pPinLevelList[i + m_nInPinNum])
				sendChange(i + m_nInPinNum);
		delete [] pOutList;
	}
}

bool KBase::appendLink(int i, KBase* p, int j)
{
	if (i < m_nInPinNum || i >= m_nPinNum ||
		!p || j < 0 || j >= p->getInPinNum())
	{
		return false;
	}
	ILink alink;
	alink.i = i;
	alink.p = p;
	alink.j = j;

	if (m_links.contains(alink))
		return false;
	m_links.append(alink);
	return true;
}

void KBase::setBoard(KBoard* pBoard)
{
	m_pBoard = pBoard;
}

void KBase::offset(const QPoint& offset)
{
	m_centerPos.setX((m_centerPos.x() + offset.x()) > 0 ? 
		(m_centerPos.x() + offset.x()) : 0);
	m_centerPos.setY((m_centerPos.y() + offset.y()) > 0 ? 
		(m_centerPos.y() + offset.y()) : 0);
}

void KBase::setCenterPos(const QPoint& pos)
{
	m_centerPos.setX(pos.x() > 0 ? pos.x() : 0);
	m_centerPos.setY(pos.y() > 0 ? pos.y() : 0);
}

QRect KBase::getBoundingRect() const
{
	return m_path.boundingRect().toRect();
}

const QPoint& KBase::getCenterPos() const
{
	return m_centerPos;
}

const QPainterPath& KBase::getPath() const
{
	return m_path;
}

const QList<KBase::IPinPos>& KBase::getPinPosList() const
{
	return m_pinPosList;
}

int KBase::getWidth() const
{
	return m_width;
}

int KBase::getHeight() const
{
	return m_height;
}

bool KBase::contains(const QPoint& pos) const
{
	return m_path.contains(pos - m_centerPos);
}

void KBase::draw(QPainter& painter) const
{
	painter.translate(m_centerPos);
	painter.drawPath(m_path);
	painter.translate(-m_centerPos);
}

void KBase::sendChange(int num)
{
	if (m_pPinLevelList[num] != HIZ)//高阻
	{
		ILink link;
		for(int i = 0; i < m_links.count(); ++i)
		{
			link = m_links.at(i);
			if (num == link.i)
			{
				link.p->setIn(link.j, m_pPinLevelList[num]);
			}
		}	
	}
}

void KBase::calculate()
{
}