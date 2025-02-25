// CSelectOptions.cpp : implementation file
//
#include "stdafx.h"

#include "SJI.h"
#include "afxdialogex.h"
#include "SelectOptions.h"


// CSelectOptions dialog

IMPLEMENT_DYNAMIC(CSelectOptions, CDialogEx)

CSelectOptions::CSelectOptions(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_OPTIONS_DIALOG, pParent)
{
	
}

CSelectOptions::~CSelectOptions()
{
}

void CSelectOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SEGMENT_COMBO, m_BumpSegmentation);
	DDX_Control(pDX, IDC_CANDIDATE_SELECTOR, m_CandidateSelector);
	DDX_Control(pDX, IDC_NNMODEL_COMBO, m_NNModel);
}


BEGIN_MESSAGE_MAP(CSelectOptions, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSelectOptions::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSelectOptions::OnBnClickedCancel)
END_MESSAGE_MAP()


// CSelectOptions message handlers


void CSelectOptions::OnBnClickedOk()
{
	m_BumpSeg = m_BumpSegmentation.GetCurSel();
	m_CandSel = m_CandidateSelector.GetCurSel();
	m_NNMod = m_NNModel.GetCurSel();



	CDialogEx::OnOK();
}


void CSelectOptions::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	CDialogEx::OnCancel();
}

BOOL CSelectOptions::OnInitDialog()
{
	BOOL ret= CDialogEx::OnInitDialog();

	m_BumpSeg = 2;
	m_BumpSegmentation.SetCurSel(m_BumpSeg);
	m_CandSel = 0;
	m_CandidateSelector.SetCurSel(m_CandSel);
	m_NNMod = 0;
	m_NNModel.SetCurSel(m_NNMod);



	return ret;
}