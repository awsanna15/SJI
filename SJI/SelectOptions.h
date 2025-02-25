#pragma once
#include "afxdialogex.h"


// CSelectOptions dialog

class CSelectOptions : public CDialogEx
{
	DECLARE_DYNAMIC(CSelectOptions)

public:
	CSelectOptions(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSelectOptions();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OPTIONS_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()


protected:
	CComboBox m_BumpSegmentation;
	CComboBox m_CandidateSelector;
	CComboBox m_NNModel;
public:
	
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();


	int m_BumpSeg;
	int m_CandSel;
	int m_NNMod;
};
