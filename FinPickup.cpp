#include "stdafx.h"
#include "FinPickup.h"
#include <stdlib.h>
#include <algorithm>
#include "Fin3DAlgorithm.h"
extern FinGlobals FG;

// ��ͷд�µ�����ע��?

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ���캯��
CollideItem::CollideItem(FinPanel * pPanel, const Fin3DRay & line, const Fin3DPoint & point, char axis):
	_line(line),
	_ptPickup(point), 
	_axis(axis)
{
	_panelPtr = pPanel;
	Parse();
}

// ��������
CollideItem::~CollideItem()
{
}

/**
*	����ʰȡ���ڰ���ϵ�λ�������и�򣬱��磬��ʰȡ���ڰ������5������6���������5���и��
**/
std::vector<tagCutDirection> CollideItem::GetCutDirectios() const
{
	std::vector<tagCutDirection> res;
	if (_panelPtr == NULL) return res;
	Fin3DCube cubePanel = _panelPtr->Cube();
	
	tagCutDirection ctDrt;
	ctDrt.pt = _collidePoint;
	ctDrt.vt = _cutDirection;
	ctDrt.pl = _collidePlaneIndex;
	res.push_back(ctDrt);

	// ���������������ײ��ǹ�����5��6�棬��ôֻ���γ�һ���и�
	if (!FinData::Fin3DAlgorithm::IsVectorsParallel(_cutDirection, cubePanel.F5()) || 
	FinData::Fin3DAlgorithm::IsPointOnCubeEdge(_collidePoint, cubePanel)) return res;

	// ��ʰȡ���ӳ���������5����6��ײʱ��Ҫ������1234�涼�����и������������
	tagCutDirection p1CutItem;
	p1CutItem.vt = cubePanel.F1();
	cubePanel.GetCubePoint(3, p1CutItem.pt);
	p1CutItem.pl = 1;
	res.push_back(p1CutItem);

	tagCutDirection p2CutItem;
	p2CutItem.vt = -cubePanel.F1();
	cubePanel.GetCubePoint(1, p2CutItem.pt);
	p2CutItem.pl = 2;
	res.push_back(p2CutItem);

	tagCutDirection p3CutItem;
	p3CutItem.vt = cubePanel.F3();
	cubePanel.GetCubePoint(5, p3CutItem.pt);
	p3CutItem.pl = 3;
	res.push_back(p3CutItem);

	tagCutDirection p4CutItem;
	p4CutItem.vt = -cubePanel.F3();
	cubePanel.GetCubePoint(1, p4CutItem.pt);
	p4CutItem.pl = 4;
	res.push_back(p4CutItem);
	
	return res;
}

// ���ز�����
bool CollideItem::operator<(const CollideItem & item)
{
	if (item.Axis() != _axis) return false;
	if (_axis == 'x') {
		if (FLOAT_LT(_collidePoint.X, item.GetCollidePoint().X)) return true;
	}
	else if (_axis == 'y') {
		if (FLOAT_LT(_collidePoint.Y, item.GetCollidePoint().Y)) return true;
	}
	else if (_axis == 'z') {
		if (FLOAT_LT(_collidePoint.Z, item.GetCollidePoint().Z)) return true;
	}

	return false;
}

/**
*	������ײ�㣬�и��
*	��ײ��Ϊֱ����������Ӵ������ʰȡ����Ǹ���
*	�и��Ϊ�������ķ���(���Ϊʰȡ��)
**/
bool CollideItem::Parse()
{
	if (_panelPtr == NULL) return false;
	Fin3DCube cubePanel = _panelPtr->Cube();
	struct collideInfo
	{
		bool		bCollide;			// ֱ���Ƿ���ƽ�潻��
		short		uPlaneIdx;			// ƽ������
		double		distan;				// ʰȡ�㵽ƽ��ľ���
		Fin3DPoint	ptCollide;			// ֱ����ƽ����ײ��

		collideInfo():bCollide(false), uPlaneIdx(0),distan(0){}
	};

	short uPlaneIdx = -1;	// ��ʰȡ������Ǹ��������
	double minDis = 0.0;
	for (int i = 0; i < 6; i++) {
		collideInfo clInfo;
		clInfo.uPlaneIdx = i;
		Fin3DPlane pl; // ������������
		cubePanel.GetCubePlane(i + 1, pl);
		FinData::FIN3D_RELATION re = FinData::Fin3DAlgorithm::RelationshipLinePlane(_line, pl, clInfo.ptCollide);
		if (re == FinData::EN_RELATION_INSECTION && cubePanel.IsPointInsideCube(clInfo.ptCollide)) {
			clInfo.bCollide = true;
			clInfo.distan = (clInfo.ptCollide - _ptPickup).Length();
			if (uPlaneIdx == -1 || FLOAT_LE(clInfo.distan, minDis)) { // ������Сֵ
				minDis = clInfo.distan;
				uPlaneIdx = i;
				_collidePlaneIndex = i + 1;
				_collidePoint = clInfo.ptCollide;
				_cutDirection = _line.Vector();
			}
		}
	}

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ���캯��
FinPickup::FinPickup(const Fin3DPoint & pt, FinProduct * product) : _ptPickup(pt), _productPtr(product)
{
	_spacePtr = NULL;
	_xR = _yR = _zR = 0.0;
	_IgnoreAssemble = NULL;
	_IgnorePanel = NULL;
}

// ��������
FinPickup::~FinPickup()
{
	for (std::map<short, FinObj<void> *>::iterator it = _limitObj.begin(); it != _limitObj.end(); it++) {
		if (it->second != NULL) {
			delete it->second;
		}
	}
}

// ��ȡʰȡ����������
Fin3DCube FinPickup::GetPickupCube()
{
	LOG_IF(INFO, FG.isLog) << __FUNCTION__;
	Fin3DCube cube; // ʰȡ�������Ĭ�ϳߴ�Ϊָ���ռ�Ĵ�С
	if (_spacePtr != NULL) {
		cube = _spacePtr->Cube();
	}
	else return cube;

	// �ж��Ƿ�����ת
	bool bRotate = false;
	if (!FLOAT_EQ(_xR, 0) || !FLOAT_EQ(_yR, 0) || !FLOAT_EQ(_zR, 0)) bRotate = true;
	if (bRotate) { // ����ת������Ҫ�ı�����ϵ
		// ��׼����ϵ
		Fin3DVector f1st(0, 1, 0);
		Fin3DVector f3st(1, 0, 0);
		Fin3DVector f5st(0, 0, 1);
		Fin3DVector f1Rotate = FinUtilParse::RbConvertAb(f1st, _xR, _yR, _zR);
		Fin3DVector f3Rotate = FinUtilParse::RbConvertAb(f3st, _xR, _yR, _zR);
		Fin3DVector f5Rotate = FinUtilParse::RbConvertAb(f5st, _xR, _yR, _zR);
		cube = cube.ChangeCoordinate(f1Rotate, f3Rotate, f5Rotate, _xR, _yR, _zR);
	}

	// ���Ĭ�ϵ����ƶ���
	AddSpaceLimit(cube);
	
	// ��ȡ������ײ����
	if (GetCollideItems() == false) return cube;
	
	// ��ȡ������ʰȡ��������Ӱ�����ײ��������
	std::vector<CollideItem> CutCollideItem;
	GetCollideItems(CutCollideItem);

	/*
	// ����޶�����
	for (std::vector<CollideItem>::iterator it = CutCollideItem.begin(); it != CutCollideItem.end(); it++) {
		CollideItem & item = *it;
		Fin3DVector vtCut = item.CutDirection();
		for (int i = 0; i < 6; i++) {
			Fin3DPlane plane;
			cube.GetCubePlane(i + 1, plane);
			if (IsVectorsParallel(vtCut, plane.PVector) && FLOAT_BT(vtCut * plane.PVector, 0)) {
				FinPanelObj * pObj = new FinPanelObj(item.GetPanel(), item.GetPanel()->Cube(), item.GetPanelLimitPlane());
				AddLimitObj(i + 1, pObj);
				LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", Add CubeLimitPanel, plane:" << i + 1 << ", PanelName:"
					<< FinUtilString::WideChar2String(item.GetPanel()->name);
				break;
			}
		}
	}*/

	// ������ײ��ϵ�г�Ĭ��������
	ModifyCube(cube, CutCollideItem);

	// ������и�������廹����ײ�İ��
	CollidePanelsModifyCube(cube);
	//TraceLimitObj();

	return cube;
}

// ����������Ŀ����
Fin3DPoint FinPickup::GetCubeAdjustCenter(double width, double depth, double height, Fin3DCube & cube)
{
	LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", width:" << width << ", depth:" << depth << ", height:" << height;
	if (_spacePtr == NULL) return Fin3DPoint();
	Fin3DCube cubeSpace = _spacePtr->Cube();

	struct tagVectDif
	{
		Fin3DVector				vt;				// ������־
		double					coorPickup;
		double					coorCenter;
		double	&				coorResCenter;

		tagVectDif(Fin3DVector vtor, double coorPic, double coorCen, double & coorRes) : vt(vtor), coorPickup(coorPic),
			coorCenter(coorCen), coorResCenter(coorRes) {}
	};

	Fin3DPoint ptResCenter;
	Fin3DPoint ptTmpCenter;
	cube.GetCubePoint(0, ptResCenter);
	cube.GetCubePoint(0, ptTmpCenter);
	tagVectDif vecMap[3] = { tagVectDif(cubeSpace.F1(), _ptPickup.Y, ptTmpCenter.Y, ptResCenter.Y), 
		tagVectDif(cubeSpace.F3(), _ptPickup.X, ptTmpCenter.X, ptResCenter.X),
		tagVectDif(cubeSpace.F5(), _ptPickup.Z, ptTmpCenter.Z, ptResCenter.Z)};

	double range = 0;
	// ���������, ������ĳ�������ݱ�ʰȡ���������峤���ҪС������Ҫ����ʰȡ����������ĳ����
	double divWidth = width - cube.Width();
	double divDepth = depth - cube.Depth();
	double divHeight = height - cube.Height();
	
	LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", Divwidth:" << divWidth<< ", Divdepth:" << divDepth << ", Divheight:" << divHeight;
	
	if (FLOAT_LT(divWidth, 0)) {
		cube.ChangeSize(divWidth, cube.F3());
		range = abs(divWidth);

		for (int i = 0; i < 3; i++) {
			if (FinData::Fin3DAlgorithm::IsVectorsParallel(cube.F3(), vecMap[i].vt) == false) continue ;
			if (FLOAT_BT(abs(vecMap[i].coorPickup - vecMap[i].coorCenter), range / 2)) {
				if (FLOAT_LT(vecMap[i].coorPickup, vecMap[i].coorCenter)) {
					vecMap[i].coorResCenter = vecMap[i].coorResCenter - range / 2;
				}
				else {
					vecMap[i].coorResCenter = vecMap[i].coorResCenter + range / 2;
				}
			}
			else {
				vecMap[i].coorResCenter = vecMap[i].coorPickup;
			}

			break;
		}
	}
	if (FLOAT_LT(divDepth, 0)) {
		cube.ChangeSize(divDepth, cube.F1());
		range = abs(divDepth);
		for (int i = 0; i < 3; i++) {
			if (FinData::Fin3DAlgorithm::IsVectorsParallel(cube.F1(), vecMap[i].vt) == false) continue ;
			if (FLOAT_BT(abs(vecMap[i].coorPickup - vecMap[i].coorCenter), range / 2)) {
				if (FLOAT_LT(vecMap[i].coorPickup, vecMap[i].coorCenter)) {
					vecMap[i].coorResCenter = vecMap[i].coorResCenter - range / 2;
				}
				else {
					vecMap[i].coorResCenter = vecMap[i].coorResCenter + range / 2;
				}
			}
			else {
				vecMap[i].coorResCenter = vecMap[i].coorPickup;
			}

			break;
		}
	}
	if (FLOAT_LT(divHeight, 0)) {
		cube.ChangeSize(divHeight, cube.F5());
		range = abs(divHeight);
		for (int i = 0; i < 3; i++) {
			if (FinData::Fin3DAlgorithm::IsVectorsParallel(cube.F5(), vecMap[i].vt) == false) continue ;
			if (FLOAT_BT(abs(vecMap[i].coorPickup - vecMap[i].coorCenter), range / 2)) {
				if (FLOAT_LT(vecMap[i].coorPickup, vecMap[i].coorCenter)) {
					vecMap[i].coorResCenter = vecMap[i].coorResCenter - range / 2;
				}
				else {
					vecMap[i].coorResCenter = vecMap[i].coorResCenter + range / 2;
				}
			}
			else {
				vecMap[i].coorResCenter = vecMap[i].coorPickup;
			}

			break;
		}
	}

	LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", ptPicCenter(" << _ptPickup.X << "," << _ptPickup.Y << "," << _ptPickup.Z << ")";
	LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", ptResCenter(" << ptResCenter.X << "," << ptResCenter.Y << "," << ptResCenter.Z << ")";

	return ptResCenter;
}

// �û��趨���ֵ����ν�ľ��ֵ����ʰȡ������������Ŀ���������Ǹ��浽���ռ��Ӧ����Ǹ�����
// ��������ʰȡ�����������(������)��һ��+���ֵ = ʰȡ��������������ĵ㵽���ռ��Ӧ��ľ���
Fin3DPoint FinPickup::CubeFixing(const Fin3DCube & cubeOut, Fin3DCube & cube, short uSurface, double dst)
{
	Fin3DPoint ptRes;
	cube.GetCubePoint(0, ptRes); // ��ȡ��ǰʰȡ����������(�³�ʰȡ������)�����ĵ�����

	if (uSurface < 3 || uSurface > 6) return ptRes;
	
	Fin3DPlane tarPlane; // Ŀ��ƽ��
	cubeOut.GetCubePlane(uSurface, tarPlane);

	// ���ʰȡ�����������ռ��������������ܺ������壬���ؼ����������ˣ����ص�ǰ���ĵ�
	bool bTilt = true;
	Fin3DVector vtCubeTesting = cube.F1();
	if (FinData::Fin3DAlgorithm::IsVectorsParallel(vtCubeTesting, cubeOut.F1()) ||
		FinData::Fin3DAlgorithm::IsVectorsParallel(vtCubeTesting, cubeOut.F3()) ||
		FinData::Fin3DAlgorithm::IsVectorsParallel(vtCubeTesting, cubeOut.F5())) {
		bTilt = false;
	}

	if (bTilt) {
		LOG(ERROR) << __FUNCTION__ << ", Tilt Cube";
		return ptRes;
	}

	Fin3DVector vctMove; // �ƶ������ķ���,��ʰȡ������λ�ò��������õ�����ʱ������Ҫ�ƶ����ƶ������ķ���Ĭ�ϴ�ʰȡ���������ĵ�
						 // ָ��Ŀ��ƽ��
	switch (uSurface) {
	case 3:
		vctMove = cubeOut.F3(); break;
	case 4:
		vctMove = -cubeOut.F3(); break;
	case 5:
		vctMove = cubeOut.F5(); break;
	case 6:
		vctMove = -cubeOut.F5(); break;
	}
	
	struct tagVctDst
	{
		Fin3DVector		vt;			// ��������
		double			dst;		// �÷�����������Ŀ�ȣ�����˵����F1�����ϣ���Ⱦ����������depth, f3ΪCube.Width

		tagVctDst(const Fin3DVector & vect, double distan) { dst = distan, vt = vect; }
	};

	tagVctDst vctDstArr[3] = { tagVctDst(cube.F1(), cube.Depth()),
		tagVctDst(cube.F3(), cube.Width()),
		tagVctDst(cube.F5(), cube.Height()) };

	double lenght;   // ��س���, ��Ҫ��Ϊ�˼���ʰȡ�����屻��ת�������
	for (int i = 0; i < 3; i++) {
		if (FinData::Fin3DAlgorithm::IsVectorsParallel(vctDstArr[i].vt, vctMove)) {
			lenght = vctDstArr[i].dst / 2;
			break;
		}
	}

	// �����������λ��Ӧ�����㣬����������ĵ���Ŀ��ƽ��ľ��� = ��ؾ��� + �趨����߾���
	// ��ǰ���������ĵ����Ŀ��ƽ��ľ���
	double curDst = FinData::Fin3DAlgorithm::Distance(ptRes, tarPlane);

	double divLength = curDst - (lenght + dst);
	double n;
	try {
		n = abs(divLength) / vctMove.Length();
	}
	catch (...) {
		LOG(ERROR) << __FUNCTION__ << ", div 0";
		n = 1;
	}

	Fin3DVector vtRes(ptRes);
	if (FLOAT_BT(divLength, 0)) { // ��Ҫ��Ŀ��ƽ�濿��
		vtRes = Fin3DVector(ptRes) + n * vctMove;
	}
	else if (FLOAT_LT(divLength, 0)) {	// ��ҪԶ��Ŀ��ƽ��
		vtRes = Fin3DVector(ptRes) + n * (-vctMove);
	}

	ptRes.X = vtRes.x;
	ptRes.Y = vtRes.y;
	ptRes.Z = vtRes.z;

	return ptRes;
}

// ��ȡ���ƶ���
std::map<short, FinObj<void> *> FinPickup::GetLimitObjs()
{
	std::map<short, FinObj<void> *> limitObjs;
	for (int i = 0; i < 6; i++) {
		std::map<short, FinObj<void> *>::iterator it = _limitObj.find(i + 1);
		if (it == _limitObj.end()) continue;
		FinObj<void> * pLimit = it->second;
		if (pLimit == NULL) continue;
		EnFreeObjType enType = pLimit->GetObjType();
		if (enType == en_FreeObj_Panel) {
			FinPanel * pPanel = static_cast<FinPanel *>(pLimit->GetInstance());
			FinPanelObj * pLimitPanel = new FinPanelObj(pPanel, pPanel->Cube(), pLimit->GetLimitPlaneIndex());
			limitObjs.insert(std::make_pair(i + 1, pLimitPanel));
		}
		else if (enType == en_FreeObj_Space) {
			FinSpace * pSpace = static_cast<FinSpace *>(pLimit->GetInstance());
			if (pSpace == NULL) continue;
			FinSpaceObj * pLimitSpace = new FinSpaceObj(pSpace, pSpace->Cube(), pLimit->GetLimitPlaneIndex());
			limitObjs.insert(std::make_pair(i + 1, pLimitSpace));
		}
		else {
			LOG(ERROR) << __FUNCTION__ << ", Error Type";
		}
	}

	return limitObjs;
}

/**
	�����������Ĭ�ϵ����ƶ���
*/
void FinPickup::AddSpaceLimit(Fin3DCube cube)
{
	if (_spacePtr == NULL) return;
	
	// ���ƿռ�������
	Fin3DCube cubeLimitSpace = _spacePtr->Cube();

	// ����������������
	for (int i = 0; i < 6; i++) {
		Fin3DPlane plane;
		cube.GetCubePlane(i + 1, plane);
		for (int n = 0; n < 6; n++) {
			Fin3DPlane limitPlane;
			cubeLimitSpace.GetCubePlane(n + 1, limitPlane);
			if (FinData::Fin3DAlgorithm::IsVectorsParallel(plane.PVector, limitPlane.PVector) && FLOAT_BT(plane.PVector * limitPlane.PVector, 0)) {
				FinObj<void> * pLimit = new FinSpaceObj(_spacePtr, cubeLimitSpace, n+1);
				AddLimitObj(i + 1, pLimit);
				LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", cubePlane:" << i + 1 << ", spacePlane:" << n + 1;
				break;
			}
		}
	}
}

// ʰȡ���̹��˸ð��
bool FinPickup::FilterPanel(FinPanel * pPanel)
{
	if (pPanel == nullptr) return true;
	if (FLOAT_LE(pPanel->geometryParams.qty, 0)) return true;
	if (pPanel->erased == true) return true;

	LOG(INFO) << __FUNCTION__ << ", PanelName:" << FinUtilString::WideChar2String(pPanel->name);

	// �ǹ����ӹ�������Ҫ���˵�
	if (pPanel->basePanelPtr != NULL && pPanel->basePanelPtr->isAssociMachine == false) return true;
	if (_IgnorePanel != nullptr && pPanel == _IgnorePanel) return true; // ���˵�ָ�����˰������
	if (_IgnoreAssemble != nullptr) {
		if (_IgnoreAssemble->IsPanelExists(pPanel->jBID)) return true; // ���˵�ָ����������ڰ������
	}

	FinObj<void> * pEfcyObj = nullptr;
	if (pPanel->IsFreeObj()) {
		pEfcyObj = _productPtr->GetFreeObj(pPanel);
		LOG(INFO) << __FUNCTION__ << ", IsFreePanelObj";
	}
	else {
		FinAssemble * pTopAsm = pPanel->GetTopAsmble();
		if (pTopAsm != nullptr && pTopAsm->IsFreeObj()) {
			pEfcyObj = _productPtr->GetFreeObj(pTopAsm);
			LOG(INFO) << __FUNCTION__ << ", Create TopAsmFreeObj";
		}
	}
	if (pEfcyObj == nullptr) return false;

	FinObj<void> * pIgnorePanelObj = _productPtr->GetFreeObj(_IgnorePanel);
	if (ExistsLimitPanel(pEfcyObj, pIgnorePanelObj)) return true;

	if (_IgnoreAssemble != nullptr) {
		FinObj<void> * pIgnoreAsmObj = _productPtr->GetFreeObj(_IgnoreAssemble);
		if (ExistsLimitPanel(pEfcyObj, pIgnoreAsmObj)) return true;
	}

	return false;
}

/*
**	��ȡ������ײ����
**	ֱ�ߺ�һ�������һ����ײ���󣬸��ݸ���ײ����ɵõ��и��
*/
bool FinPickup::GetCollideItems()
{
	LOG_IF(INFO, FG.isLog) << __FUNCTION__;
	if (_productPtr == nullptr || _spacePtr == nullptr) return false;

	_collideItemVectorX.clear();
	_collideItemVectorY.clear();
	_collideItemVectorZ.clear();

	// Ŀ��ռ��ھ���ʰȡ�������������������
	Fin3DRay xPst(Fin3DVector(_spacePtr->f3), _ptPickup);	// x ������
	Fin3DRay xNgt = -xPst;
	Fin3DRay yPst(Fin3DVector(_spacePtr->f1), _ptPickup);
	Fin3DRay yNgt = -yPst;
	Fin3DRay zPst(Fin3DVector(_spacePtr->f5), _ptPickup);
	Fin3DRay zNgt = -zPst;


	// ������Ʒ����
	std::list<FinPanel *>::iterator itPanel;
	for (itPanel = _productPtr->panels.begin(); itPanel != _productPtr->panels.end(); itPanel++) {
		FinPanel * pPanel = *itPanel;
		if (FilterPanel(pPanel)) continue;

		LOG(INFO) << __FUNCTION__ << ", PanelCollide, PanelName:" << FinUtilString::WideChar2String(pPanel->name);

		Fin3DCube cubePanel = pPanel->Cube(); // ���������
		if (FinData::Fin3DAlgorithm::CLCubeRay(cubePanel, xPst)) {
			_collideItemVectorX.push_back(CollideItem(pPanel, xPst, _ptPickup, 'x'));
		}
		if (FinData::Fin3DAlgorithm::CLCubeRay(cubePanel, xNgt)) {
			_collideItemVectorX.push_back(CollideItem(pPanel, xNgt, _ptPickup, 'x'));
		}
		if (FinData::Fin3DAlgorithm::CLCubeRay(cubePanel, yPst)) {
			_collideItemVectorY.push_back(CollideItem(pPanel, yPst, _ptPickup, 'y'));
		}
		if (FinData::Fin3DAlgorithm::CLCubeRay(cubePanel, yNgt)) {
			_collideItemVectorY.push_back(CollideItem(pPanel, yNgt, _ptPickup, 'y'));
		}
		if (FinData::Fin3DAlgorithm::CLCubeRay(cubePanel, zPst)) {
			_collideItemVectorZ.push_back(CollideItem(pPanel, zPst, _ptPickup, 'z'));
		}
		if (FinData::Fin3DAlgorithm::CLCubeRay(cubePanel, zNgt)) {
			_collideItemVectorZ.push_back(CollideItem(pPanel, zNgt, _ptPickup, 'z'));
		}
	}

	// �����Ϣ
	std::vector<CollideItem>::iterator itItem;
	LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", x axis Items:";
	for (itItem = _collideItemVectorX.begin(); itItem != _collideItemVectorX.end(); itItem++) {
		char szCollideItemInfo[128] = { 0 };
		sprintf_s(szCollideItemInfo, "%s, %s", __FUNCTION__, FinUtilString::WideChar2String((*itItem).GetPanel()->name).c_str());
		LOG_IF(INFO, FG.isLog) << szCollideItemInfo;
	}
	LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", y axis Items:";
	for (itItem = _collideItemVectorY.begin(); itItem != _collideItemVectorY.end(); itItem++) {
		char szCollideItemInfo[128] = { 0 };
		sprintf_s(szCollideItemInfo, "%s, %s", __FUNCTION__, FinUtilString::WideChar2String((*itItem).GetPanel()->name).c_str());
		LOG_IF(INFO, FG.isLog) << szCollideItemInfo;
	}
	LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", z axis Items:";
	for (itItem = _collideItemVectorZ.begin(); itItem != _collideItemVectorZ.end(); itItem++) {
		char szCollideItemInfo[128] = { 0 };
		sprintf_s(szCollideItemInfo, "%s, %s", __FUNCTION__, FinUtilString::WideChar2String((*itItem).GetPanel()->name).c_str());
		LOG_IF(INFO, FG.isLog) << szCollideItemInfo;
	}

	return true;
}

/**
*	��ȡֱ��Ӱ�����������ײ��������
*	��������֮���Ѿ�����ײ������һ����˳�����кã���ײ������и���Ǵ�ʰȡ�㴩�����ָ������
*   ����������õ���ײ�����������ҵ��и���෴����ײ������ҵ���ʰȡ���λ��
**/ 
bool FinPickup::GetCollideItems(OUT std::vector<CollideItem> & arrary)
{
	// �������������ϵ���ײ����
	std::sort(_collideItemVectorX.begin(), _collideItemVectorX.end());
	std::sort(_collideItemVectorY.begin(), _collideItemVectorY.end());
	std::sort(_collideItemVectorZ.begin(), _collideItemVectorZ.end());

	std::vector<CollideItem>::iterator itItem;
	std::vector<CollideItem>::iterator itNext;
	std::vector<CollideItem>::iterator itBegin;
	for (itItem = _collideItemVectorX.begin(); itItem != _collideItemVectorX.end(); itItem++) {
		itNext = itItem;
		itNext++;
		if (itNext == _collideItemVectorX.end()) { // �Ѿ�������β��, ˵���ڸ�������������ײ����λ��ʰȡ���һ��
													// ���������ϵ��ʰȡ��ȽϿ����ն�ʰȡ��Ӱ��������ͷ����β������һ������
			LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", sameDirection";
			itBegin = _collideItemVectorX.begin();

			Fin3DPoint ptBegin = (*itBegin).GetCollidePoint();
			Fin3DPoint ptItem = (*itItem).GetCollidePoint();

			if (FLOAT_EQ(ptBegin.X, ptItem.X)) { // ͷβ���, ���ȡͷ����β��Ŀǰ�̶�β��
				arrary.push_back(*itItem);
				break;
			}
		
			// ����̶���С��������
			if (FLOAT_BE(_ptPickup.X, ptItem.X)) { // ���ʰȡ����ڵ���β��
				arrary.push_back(*itItem);
			}
			else { // ʰȡ��С����Сֵ��˵��������ײλ��ʰȡ���Ҳ� (x ������)
				arrary.push_back(*itBegin);
			}
			break;
		}
		Fin3DVector vtNext = (*itNext).CutDirection();
		Fin3DVector vtItem = (*itItem).CutDirection();
		if (FLOAT_BT(vtNext * vtItem, 0)) continue; // ���˵�ͬ��
		arrary.push_back(*itItem);
		arrary.push_back(*itNext);
		break;
	}
	for (itItem = _collideItemVectorY.begin(); itItem != _collideItemVectorY.end(); itItem++) {
		itNext = itItem;
		itNext++;
		if (itNext == _collideItemVectorY.end()) {
			itBegin = _collideItemVectorY.begin();

			Fin3DPoint ptBegin = (*itBegin).GetCollidePoint();
			Fin3DPoint ptItem = (*itItem).GetCollidePoint();

			if (FLOAT_EQ(ptBegin.Y, ptItem.Y)) { // ͷβ���, ���ȡͷ����β��Ŀǰ�̶�β��
				arrary.push_back(*itItem);
				break;
			}

			// ����̶���С��������
			if (FLOAT_BE(_ptPickup.Y, ptItem.Y)) { // ���ʰȡ����ڵ���β��
				arrary.push_back(*itItem);
			}
			else { // ʰȡ��С����Сֵ��˵��������ײλ��ʰȡ���Ҳ� (x ������)
				arrary.push_back(*itBegin);
			}
			break;
		}
		Fin3DVector vtNext = (*itNext).CutDirection();
		Fin3DVector vtItem = (*itItem).CutDirection();
		if (FLOAT_BT(vtNext * vtItem, 0)) continue;
		arrary.push_back(*itItem);
		arrary.push_back(*itNext);
		break;
	}
	for (itItem = _collideItemVectorZ.begin(); itItem != _collideItemVectorZ.end(); itItem++) {
		itNext = itItem;
		itNext++;
		Fin3DVector vtItem = (*itItem).CutDirection();
		if (itNext == _collideItemVectorZ.end()) {
			itBegin = _collideItemVectorZ.begin();

			Fin3DPoint ptBegin = (*itBegin).GetCollidePoint();
			Fin3DPoint ptItem = (*itItem).GetCollidePoint();

			if (FLOAT_EQ(ptBegin.Z, ptItem.Z)) { // ͷβ���, ���ȡͷ����β��Ŀǰ�̶�β��
				arrary.push_back(*itItem);
				break;
			}

			// ����̶���С��������
			if (FLOAT_BE(_ptPickup.Z, ptItem.Z)) { // ���ʰȡ����ڵ���β��
				arrary.push_back(*itItem);
			}
			else { // ʰȡ��С����Сֵ��˵��������ײλ��ʰȡ���Ҳ� (x ������)
				arrary.push_back(*itBegin);
			}		
			break;
		}
		Fin3DVector vtNext = (*itNext).CutDirection();
		if (FLOAT_BT(vtNext * vtItem, 0)) continue;
		arrary.push_back(*itItem);
		arrary.push_back(*itNext);
		break;
	}

	LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", EfficacyPanel:";
	for (itItem = arrary.begin(); itItem != arrary.end(); itItem++) {
		char szPanelName[128] = { 0 };
		sprintf_s(szPanelName, "%s, %s", __FUNCTION__, FinUtilString::WideChar2String((*itItem).GetPanel()->name).c_str());
		LOG_IF(INFO, FG.isLog) << szPanelName;
	}

	return true;
}

// ������ײ�����и�������
bool FinPickup::ModifyCube(OUT Fin3DCube & cube, std::vector<CollideItem> & arrary)
{
	LOG_IF(INFO, FG.isLog) << __FUNCTION__;
	std::vector<CollideItem>::iterator itItem;
	for (itItem = arrary.begin(); itItem != arrary.end();) {
		FinPanel * pPanel = itItem->GetPanel();
		if (pPanel == NULL) { // ��ײ������һ���а��
			itItem = arrary.erase(itItem);
			continue;
		}
	
		if (Cut(cube, *itItem) == false) return false;

		itItem++;
	}
	return true;
}

// �и�������
bool FinPickup::Cut(OUT Fin3DCube & cube, const CollideItem & item)
{
	LOG_IF(INFO, FG.isLog) << __FUNCTION__;
	std::vector<tagCutDirection> cutArr = item.GetCutDirectios();
	std::vector<tagCutDirection>::iterator itCut;
	LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", cutArr.size:" << cutArr.size();
	for (itCut = cutArr.begin(); itCut != cutArr.end(); itCut++) {
		tagCutDirection & ct = *itCut;
		LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", dir(" << (*itCut).vt.x << "," << (*itCut).vt.y << "," << (*itCut).vt.z << ")"
			<< ", pt:(" << ct.pt.X << "," << ct.pt.Y << "," << ct.pt.Z << ")";
		Fin3DVector vtCut = GetCutVector(cube, *itCut);
		char szCutVector[64] = { 0 };
		sprintf_s(szCutVector, "CutVectors(%.3lf,%.3lf,%.3lf)", vtCut.x, vtCut.y, vtCut.z);
		LOG_IF(INFO, FG.isLog) << __FUNCTION__ << "," << szCutVector;

		if (vtCut.IsZeroVector()) continue;
		cube.Cut(vtCut);

		// ������ƶ���
		for (int i = 0; i < 6; i++) {
			Fin3DPlane plane;
			cube.GetCubePlane(i + 1, plane);
			if (FinData::Fin3DAlgorithm::IsVectorsParallel((*itCut).vt, plane.PVector) && FLOAT_BT((*itCut).vt * plane.PVector, 0)) {
				FinPanelObj * pLimit = new FinPanelObj(item.GetPanel(), item.GetPanel()->Cube(), (*itCut).pl);
				AddLimitObj(i + 1, pLimit);
				break;
			}
		}
	}
	return true;
}

/**
*	��ȡ�и�����
*	�и��������ʱ�������϶��Ǵ������������ָ������ģ������ǰ��
*	tagCutDirection �еı�ʾ�����������������ʵ�ʵ��и�����෴�ģ���tagCutDirection�еĵ㵽�и�ƽ���������ƽ��ķ����������������и
*	��Ϊ�и�������������ⲿ
*	����tagCutDirection�е��и��ȷ���и��棬Ȼ������㵽��ƽ�����������Ϊͬ������, ��ȡ���и�㵽�и����������Ϊ����
**/
Fin3DVector FinPickup::GetCutVector(const Fin3DCube & cube, const tagCutDirection & dir)
{
	Fin3DVector vtRes;
	Fin3DPlane basPlane;	// ���и����
	if (FinData::Fin3DAlgorithm::IsVectorsParallel(cube.F1(), dir.vt)) { // ��f1ƽ��
		if (FLOAT_BT(dir.vt * cube.F1(), 0)) { // ��f1ͬ��
			cube.GetCubePlane(1, basPlane);
		}
		else {
			Fin3DPlane plane2;
			cube.GetCubePlane(2, basPlane);
		}
	}
	else if (FinData::Fin3DAlgorithm::IsVectorsParallel(cube.F3(), dir.vt)) {
		if (FLOAT_BT(dir.vt * cube.F3(), 0)) {
			cube.GetCubePlane(3, basPlane);
		}
		else {
			cube.GetCubePlane(4, basPlane);
		}
	}
	else if (FinData::Fin3DAlgorithm::IsVectorsParallel(cube.F5(), dir.vt)) {
		if (FLOAT_BT(dir.vt * cube.F5(), 0)) {
			cube.GetCubePlane(5, basPlane);
		}
		else {
			cube.GetCubePlane(6, basPlane);
		}
	}

	Fin3DVector vtCut = FinData::Fin3DAlgorithm::VectorToPlane(dir.pt, basPlane); // ��׼��ָ���и��������
	if (FLOAT_BT(basPlane.Vector() * vtCut, 0)) {
		vtRes = -vtCut;
	}

	return vtRes;
}

/*
*	����������������ײʱ����Ҫ�ð������1��2 ����3��4 ȥ�и�������
*/
bool FinPickup::CollidePanelsModifyCube(Fin3DCube & cube)
{
	if (_productPtr == NULL) {
		LOG(ERROR) << __FUNCTION__ << ", ProductPtr == null";
		return false;
	}

	std::list<FinPanel *>::iterator itProductPanel;
	for (itProductPanel = _productPtr->panels.begin(); itProductPanel != _productPtr->panels.end(); itProductPanel++) {
		FinPanel * pProductPanel = *itProductPanel;
		if (FilterPanel(pProductPanel)) continue;

		double dir = cube.F1() * pProductPanel->f1;
		if (!FinData::Fin3DAlgorithm::IsVectorsParallel(cube.F1(), pProductPanel->f1) && FLOAT_NEQ(dir, 0)) continue;  // ���˵����������

		Fin3DCube cubePanel = pProductPanel->Cube();
		if (FinData::Fin3DAlgorithm::IsCubesCollide(cubePanel, cube) == false) continue;

		LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", PanelCollideCube, PanelName:" << FinUtilString::WideChar2String(pProductPanel->name);

		// ��1��2 �� ��3��4
		for (int uPairCount = 0; uPairCount < 3; uPairCount = uPairCount+2) {
			Fin3DPlane plane1;
			Fin3DPlane plane2;
			cubePanel.GetCubePlane(uPairCount + 1, plane1);
			cubePanel.GetCubePlane(uPairCount + 2, plane2);
			// ���ʰȡ�㲻�ڰ����1����2(����3��4)����ƽ��ķ�Χ��
			if (FinData::Fin3DAlgorithm::IsPtBetweenPlanes(_ptPickup, plane1, plane2)) continue;
			LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", pickupPoint not between panelPlanes:" << ((uPairCount == 0) ? "12":"34");
			double d1 = FinData::Fin3DAlgorithm::Distance(_ptPickup, plane1); // ʰȡ�㵽��ľ���
			double d2 = FinData::Fin3DAlgorithm::Distance(_ptPickup, plane2); // ʰȡ�㵽��ľ���

			short uPanelnBasePt;			// ����и����ϵ�ĳ������
			Fin3DPlane basePanelPlane;		// ���ĳ���и���
			short uLimitPlane;				// ���������

			LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", d1:" << d1 << ",d2:" << d2;
			// �Ƚ�ʰȡ�㵽��ľ���ȷ���и�����и��
			if (FLOAT_BT(d1, d2)) {
				uPanelnBasePt = 1;
				basePanelPlane = plane2;
				uLimitPlane = uPairCount + 2;
			}
			else {
				uPanelnBasePt = 5;
				basePanelPlane = plane1;
				uLimitPlane = uPairCount + 1;
			}

			Fin3DVector cutVector;			// ��1����2����������и�����
			Fin3DPoint ptCut;				// ����и���������һ�㣬�����и���Ϊ��2����ȡ�������1278�е�����һ����
			cubePanel.GetCubePoint(uPanelnBasePt, ptCut);
			Fin3DVector vtPickupPlane;
			if (FLOAT_EQ(FinData::Fin3DAlgorithm::Distance(_ptPickup, basePanelPlane), 0)) {		// ���ʰȡ�����и�ƽ���ϣ���ȡƽ�������෴����������Ϊ���������е�����������ָ����ߵ�
				vtPickupPlane = -basePanelPlane.PVector;
			}
			else {
				vtPickupPlane = FinData::Fin3DAlgorithm::VectorToPlane(_ptPickup, basePanelPlane);	// ʰȡ�㵽����и��������
			}
			for (int i = 0; i < 6; i++) {
				Fin3DPlane plane;
				cube.GetCubePlane(i + 1, plane);
				Fin3DVector vtPickupCube = FinData::Fin3DAlgorithm::VectorToPlane(ptCut, plane);	// ����ϵ��и�㵽���������������ϵ�����
				if (FLOAT_BT(vtPickupCube * vtPickupPlane, 0)) {		// ��ʰȡ�㵽�и�����������и����ϵĵ㵽������ĳ�����ϵ�����ͬ�򣬾��ҵ����и�����
					cutVector = -vtPickupCube;
					LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", CubePlaneIndex:" << i + 1;
					break;
				}
			}
			LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", CutVector:" << cutVector.x << "," << cutVector.y << "," << cutVector.z;
			cube.Cut(cutVector);

			// ������ƶ���
			for (int i = 0; i < 6; i++) {
				Fin3DPlane plane;
				cube.GetCubePlane(i + 1, plane);
				if (FinData::Fin3DAlgorithm::IsVectorsParallel(cutVector, plane.PVector) && FLOAT_LE(cutVector * plane.PVector, 0)) {
					FinPanelObj * pLimit = new FinPanelObj(pProductPanel, pProductPanel->Cube(), uLimitPlane);
					AddLimitObj(i + 1, pLimit);
					LOG_IF(INFO, FG.isLog) << ", Add limitPanel, plane:" << i + 1 << ", panelName:"
						<< FinUtilString::WideChar2String(pProductPanel->name);
					break;
				}
			}
		}
	}

	return true;
}

// ��������ƶ��� uPlane 1-6
bool FinPickup::AddLimitObj(short uPlane, FinObj<void> * limitObj)
{
	std::map<short, FinObj<void> *>::iterator itExists = _limitObj.find(uPlane);
	if (itExists != _limitObj.end()) {
		FinObj<void> * pExistLimit = itExists->second;
		if (pExistLimit != NULL) delete pExistLimit;
		_limitObj.erase(itExists);
	}

	_limitObj.insert(std::make_pair(uPlane, limitObj));

	if (limitObj->GetObjType() == en_FreeObj_Panel) {
		FinPanel * pLimit = static_cast<FinPanel*>(limitObj->GetInstance());
		LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", Add Limit, planeIndex:" << uPlane << ", PanelName:"
			<< FinUtilString::WideChar2String(pLimit->name);
	}
	else if (limitObj->GetObjType() == en_FreeObj_Space) {
		FinSpace * pLimit = static_cast<FinSpace *>(limitObj->GetInstance());
		LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", Add Limit, planeIndex:" << uPlane << ", SpaceName:"
			<< FinUtilString::WideChar2String(pLimit->name);
	}
	return true;
}

void FinPickup::EraseLimitObj(short uPlane)
{
	std::map<short, FinObj<void> *>::iterator it = _limitObj.find(uPlane);
	if (it == _limitObj.end()) return;

	FinObj<void> * pLimitObj = it->second;
	_limitObj.erase(it);
	delete pLimitObj;
}

void FinPickup::EraseLimitObj(Fin3DCube & cube, const Fin3DVector & vt)
{
	short uPlaneIndex = 0;
	Fin3DVector vtArr[6] = { cube.F1(), -cube.F1(), cube.F3(), -cube.F3(), cube.F5(), -cube.F5() };
	for (int i = 0; i < 6; i++) {
		if (FinData::Fin3DAlgorithm::IsVectorsParallel(vtArr[i], vt) && FLOAT_BT(vtArr[i] * vt, 0)) {
			uPlaneIndex = i + 1;
			break;
		}
	}

	EraseLimitObj(uPlaneIndex);
}

bool FinPickup::GetLimitPlane(short uIndex, Fin3DPlane & plane)
{
	std::map<short, FinObj<void> *>::iterator it = _limitObj.find(uIndex);
	if (it == _limitObj.end()) return false;

	FinObj<void>* pLimitObj = it->second;
	Fin3DCube cubeLimit = pLimitObj->GetObjCube();
	int nLmtPlaneIdx = pLimitObj->GetLimitPlaneIndex();
	cubeLimit.GetCubePlane(nLmtPlaneIdx, plane);

	return true;
}

void FinPickup::AdjustLimitObj(Fin3DCube & cubeSpace, Fin3DCube & cubePickup, int surfaceIdx)
{
	Fin3DVector vtSurface;
	switch (surfaceIdx) {
	case 1: vtSurface = cubeSpace.F1();		break;
	case 2: vtSurface = -cubeSpace.F1();	break;
	case 3: vtSurface = cubeSpace.F3();		break;
	case 4: vtSurface = -cubeSpace.F3();	break;
	case 5: vtSurface = cubeSpace.F5();		break;
	case 6: vtSurface = -cubeSpace.F5();	break;
	}

	// ȷ��ʰȡ������f1�����������
	Fin3DPlane planeA;
	Fin3DPlane planeB;
	if (GetLimitPlane(1, planeA) && GetLimitPlane(2, planeB)) {
		Fin3DPoint pt1;
		Fin3DPoint pt3;
		cubePickup.GetCubePoint(3, pt3);
		cubePickup.GetCubePoint(1, pt1);
		double intval1 = FinData::Fin3DAlgorithm::Distance(pt3, planeA); // ʰȡ��������������1�ľ���
		double intval2 = FinData::Fin3DAlgorithm::Distance(pt1, planeB); // ʰȡ��������������2�ľ���
		if (FLOAT_BT(intval1, 0) || FLOAT_BT(intval2, 0)) {
			if (vtSurface.IsZeroVector() == false) {
				EraseLimitObj(cubePickup, -vtSurface);
			}
			else if (FLOAT_BT(intval1, intval2)) {
				EraseLimitObj(1);
			}
			else {
				EraseLimitObj(2);
			}
		}
	}

	if (GetLimitPlane(3, planeA) && GetLimitPlane(4, planeB)) {
		Fin3DPoint pt5;
		Fin3DPoint pt1;
		cubePickup.GetCubePoint(5, pt5);
		cubePickup.GetCubePoint(1, pt1);
		double intval3 = FinData::Fin3DAlgorithm::Distance(pt5, planeA); // ʰȡ��������������1�ľ���
		double intval4 = FinData::Fin3DAlgorithm::Distance(pt1, planeB); // ʰȡ��������������2�ľ���
		if (FLOAT_BT(intval3, 0) || FLOAT_BT(intval4, 0)) {
			if (vtSurface.IsZeroVector() == false) {
				EraseLimitObj(cubePickup, -vtSurface);
			}
			else if (FLOAT_BT(intval3, intval4)) {
				EraseLimitObj(3);
			}
			else {
				EraseLimitObj(4);
			}
		}
	}
	
	if (GetLimitPlane(5, planeA) && GetLimitPlane(6, planeB)) {
		Fin3DPoint pt6;
		Fin3DPoint pt1;
		cubePickup.GetCubePoint(6, pt6);
		cubePickup.GetCubePoint(1, pt1);
		double intval5 = FinData::Fin3DAlgorithm::Distance(pt6, planeA);// ʰȡ��������������1�ľ���
		double intval6 = FinData::Fin3DAlgorithm::Distance(pt1, planeB); // ʰȡ��������������2�ľ���
		if (FLOAT_BT(intval5, 0) || FLOAT_BT(intval6, 0)) {
			if (vtSurface.IsZeroVector() == false) {
				EraseLimitObj(cubePickup, -vtSurface);
			}
			else if (FLOAT_BT(intval5, intval6)) {
				EraseLimitObj(5);
			}
			else {
				EraseLimitObj(6);
			}
		}
	}

	TraceLimitObj();
}

// ������ƶ��󣬵�����
void FinPickup::TraceLimitObj()
{
	for (int i=0; i<6; i++) {
		std::map<short, FinObj<void>*>::iterator it = _limitObj.find(i + 1);
		if (it == _limitObj.end()) continue;
		FinObj<void> * pObj = it->second;
		if (pObj == NULL) continue;
		if (pObj->GetObjType() == en_FreeObj_Panel) {
			FinPanel * pLimitPanel = static_cast<FinPanel *>(pObj->GetInstance());
			if (pLimitPanel != NULL) {
				LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", plane:" << it->first << ", LimitPanelName:" << FinUtilString::WideChar2String(pLimitPanel->name)
					<< ", limitPlaneIndex:" << pObj->GetLimitPlaneIndex();
			}
		}
		else if (pObj->GetObjType() == en_FreeObj_Space) {
			FinSpace * pLimitSpace = static_cast<FinSpace *>(pObj->GetInstance());
			if (pLimitSpace != NULL) {
				LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", plane:" << it->first << ", LimitSpaceName:" << FinUtilString::WideChar2String(pLimitSpace->name)
					<< ", limitPlaneIndex:" << pObj->GetLimitPlaneIndex();
			}
		}
		else {
			LOG_IF(INFO, FG.isLog) << __FUNCTION__ << ", LimitType:" << pObj->GetObjType();
		}
	}
}

bool FinPickup::ExistsLimitPanel(FinObj<void> * pFreeObj, FinObj<void> * pTarFreeObj)
{
	if (pTarFreeObj == nullptr || pFreeObj == nullptr) return false;

	FinObj<void> * pObj = pFreeObj;
	if (pFreeObj->GetObjType() == en_FreeObj_Panel) {
		FinPanel * pCenter = static_cast<FinPanel*>(pFreeObj->GetInstance());
		FinAssemble * pTopAsm = pCenter->GetTopAsmble();
		if (pTopAsm != nullptr) {
			FinObj<void> * pFreeAsmObj = _productPtr->GetFreeObj(pTopAsm);
			if (pFreeObj != nullptr) {
				pObj = pFreeAsmObj;
			}
		}
	}

	LOG(INFO) << __FUNCTION__ << ", FilterPanelName:" << FinUtilString::WideChar2String(pFreeObj->Name())
		<< ", TarObjName:" << FinUtilString::WideChar2String(pTarFreeObj->Name());

	// ��������������ƶ���
	for (int i = 1; i <= 6; i++) {
		FinObj<void> * pLimitObj = pObj->GetPlaneLimitObj(i);
		if (pLimitObj != nullptr) {
			FinObj<void> * pNext = nullptr;
			LOG(INFO) << __FUNCTION__ << ", planeIdx:" << i << ", limitObjName:" << FinUtilString::WideChar2String(pLimitObj->Name());
			if (*pLimitObj == *pTarFreeObj) return true;

			if (pLimitObj->GetObjType() == en_FreeObj_Panel) {
				FinPanel * pPanel = static_cast<FinPanel *>(pLimitObj->GetInstance());
				if (pPanel != nullptr) {
					if (pPanel->IsFreeObj()) pNext = _productPtr->GetFreeObj(pPanel);
					FinAssemble * pTpAsm = pPanel->GetTopAsmble();
					if (pTpAsm != nullptr) {
						if (pTpAsm->IsFreeObj()) {
							FinObj<void> * pFreeAsm = _productPtr->GetFreeObj(pTpAsm);
							if (pFreeAsm != nullptr) {
								if (*pFreeAsm == *pTarFreeObj) return true;
								pNext = pFreeAsm;
							}
						}
					}
				}
			}

			if (ExistsLimitPanel(pNext, pTarFreeObj)) return true;
		}
	}
	
	return false;
}
