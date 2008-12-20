// Copyright (C) 2003-2008 Dolphin Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official SVN repository and contact information can be found at
// http://code.google.com/p/dolphin-emu/

#include "Globals.h"
#include "MemcardManager.h"
#include "Common.h"
#include "wx/mstream.h"

const u8 hdr[] = {
0x42,0x4D,
0x38,0x30,0x00,0x00,
0x00,0x00,0x00,0x00,
0x36,0x00,0x00,0x00,
0x28,0x00,0x00,0x00,
0x20,0x00,0x00,0x00, //W
0x20,0x00,0x00,0x00, //H
0x01,0x00,
0x20,0x00,
0x00,0x00,0x00,0x00,
0x02,0x30,0x00,0x00, //data size
0x12,0x0B,0x00,0x00,
0x12,0x0B,0x00,0x00,
0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00
};

wxBitmap wxBitmapFromMemoryRGBA(const unsigned char* data, int width, int height)
{
	int stride = (4*width);

	int bytes = (stride*height) + sizeof(hdr);

	bytes = (bytes+3)&(~3);

	u8 *pdata = new u8[bytes];

	memset(pdata,0,bytes);
	memcpy(pdata,hdr,sizeof(hdr));

	u8 *pixelData = pdata + sizeof(hdr);

	for (int y=0;y<height;y++)
	{
		memcpy(pixelData+y*stride,data+(height-y-1)*stride,stride);
	}

	*(int*)(pdata+18) = width;
	*(int*)(pdata+22) = height;
	*(u32*)(pdata+34) = bytes-sizeof(hdr);

	wxMemoryInputStream is(pdata, bytes);
	wxBitmap map(wxImage(is, wxBITMAP_TYPE_BMP, -1), -1);

	delete [] pdata;

	return map;
}

BEGIN_EVENT_TABLE(CMemcardManager, wxDialog)
	EVT_CLOSE(CMemcardManager::OnClose)

	EVT_BUTTON(ID_COPYFROM_A,CMemcardManager::CopyDeleteClick)
	EVT_BUTTON(ID_COPYFROM_B,CMemcardManager::CopyDeleteClick)
	EVT_BUTTON(ID_FIXCHECKSUM_A,CMemcardManager::CopyDeleteClick)
	EVT_BUTTON(ID_FIXCHECKSUM_B,CMemcardManager::CopyDeleteClick)
	EVT_BUTTON(ID_DELETE_A,CMemcardManager::CopyDeleteClick)
	EVT_BUTTON(ID_DELETE_B,CMemcardManager::CopyDeleteClick)
	EVT_BUTTON(ID_SAVEIMPORT_B,CMemcardManager::CopyDeleteClick)
	EVT_BUTTON(ID_SAVEEXPORT_B,CMemcardManager::CopyDeleteClick)
	EVT_BUTTON(ID_SAVEIMPORT_A,CMemcardManager::CopyDeleteClick)
	EVT_BUTTON(ID_SAVEEXPORT_A,CMemcardManager::CopyDeleteClick)
	EVT_BUTTON(ID_CONVERTTOGCI,CMemcardManager::CopyDeleteClick)
	EVT_BUTTON(ID_PREVPAGE_A, CMemcardManager::OnPageChange)
	EVT_BUTTON(ID_NEXTPAGE_A, CMemcardManager::OnPageChange)
	EVT_BUTTON(ID_PREVPAGE_B, CMemcardManager::OnPageChange)
	EVT_BUTTON(ID_NEXTPAGE_B, CMemcardManager::OnPageChange)

	EVT_FILEPICKER_CHANGED(ID_MEMCARDPATH_A,CMemcardManager::OnPathChange)
	EVT_FILEPICKER_CHANGED(ID_MEMCARDPATH_B,CMemcardManager::OnPathChange)

	EVT_MENU(ID_USEPAGES, CMemcardManager::OnMenuChange)
	EVT_MENU_RANGE(ID_COPYFROM_A, ID_CONVERTTOGCI, CMemcardManager::CopyDeleteClick)
	EVT_MENU_RANGE(ID_NEXTPAGE_A, ID_PREVPAGE_B, CMemcardManager::OnPageChange)
	EVT_MENU_RANGE(COLUMN_BANNER, NUMBER_OF_COLUMN, CMemcardManager::OnMenuChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(CMemcardManager::CMemcardListCtrl, wxListCtrl)
	EVT_RIGHT_DOWN(CMemcardManager::CMemcardListCtrl::OnRightClick)
END_EVENT_TABLE()

CMemcardManager::CMemcardManager(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& position, const wxSize& size, long style)
	: wxDialog(parent, id, title, position, size, style)
{
	memoryCard[SLOT_A]=NULL;
	memoryCard[SLOT_B]=NULL;
	if (MemcardManagerIni.Load(CONFIG_FILE))
	{
		MemcardManagerIni.Get("MemcardManager", "Items per page",  &itemsPerPage, 16);
	}
	else itemsPerPage = 16;
	maxPages = (128 / itemsPerPage) - 1;
	CreateGUIControls();
}

CMemcardManager::~CMemcardManager()
{
	if (memoryCard[SLOT_A])
	{
		delete memoryCard[SLOT_A];
		memoryCard[SLOT_A] = NULL;
	}
	if (memoryCard[SLOT_B])
	{
		delete memoryCard[SLOT_B];
		memoryCard[SLOT_B] = NULL;
	}
	MemcardManagerIni.Load(CONFIG_FILE);
	MemcardManagerIni.Set("MemcardManager", "Items per page",  itemsPerPage);
	MemcardManagerIni.Save(CONFIG_FILE);
}

CMemcardManager::CMemcardListCtrl::CMemcardListCtrl(wxWindow* parent, const wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
	: wxListCtrl(parent, id, pos, size, style)
{
	if (MemcardManagerIni.Load(CONFIG_FILE))
	{
		MemcardManagerIni.Get("MemcardManager", "Use Pages", &usePages, true);
		MemcardManagerIni.Get("MemcardManager", "cBanner", &column[COLUMN_BANNER], true);
		MemcardManagerIni.Get("MemcardManager", "cTitle", &column[COLUMN_TITLE], true);
		MemcardManagerIni.Get("MemcardManager", "cComment", &column[COLUMN_COMMENT], true);
		MemcardManagerIni.Get("MemcardManager", "cBlocks", &column[COLUMN_BLOCKS], true);
		MemcardManagerIni.Get("MemcardManager", "cBanner", &column[COLUMN_BANNER], true);
		MemcardManagerIni.Get("MemcardManager", "cFirst Block", &column[COLUMN_FIRSTBLOCK], true);
	}
	else
	{
		usePages = true;
		for (int i = 0; i < NUMBER_OF_COLUMN; i++) column[i] = true;
	}
	twoCardsLoaded = false;
	prevPage = false;
	nextPage = false;

}

CMemcardManager::CMemcardListCtrl::~CMemcardListCtrl()
{
	MemcardManagerIni.Load(CONFIG_FILE);
	MemcardManagerIni.Set("MemcardManager", "Use Pages", usePages);
	MemcardManagerIni.Set("MemcardManager", "cBanner", column[COLUMN_BANNER]);
	MemcardManagerIni.Set("MemcardManager", "cTitle", column[COLUMN_TITLE]);
	MemcardManagerIni.Set("MemcardManager", "cComment", column[COLUMN_COMMENT]);
	MemcardManagerIni.Set("MemcardManager", "cBlocks", column[COLUMN_BLOCKS]);
	MemcardManagerIni.Set("MemcardManager", "cBanner", column[COLUMN_BANNER]);
	MemcardManagerIni.Set("MemcardManager", "cFirst Block", column[COLUMN_FIRSTBLOCK]);
	MemcardManagerIni.Save(CONFIG_FILE);
}

void CMemcardManager::CreateGUIControls()
{
	// Create the controls for both memcards
	// Loading invalid .raw files should no longer crash the app
	m_MemcardPath[SLOT_A] = new wxFilePickerCtrl(this, ID_MEMCARDPATH_A, wxEmptyString, wxT("Choose a memory card:"),
		wxT("Raw memcards (*.raw)|*.raw"), wxDefaultPosition, wxDefaultSize, wxFLP_USE_TEXTCTRL|wxFLP_OPEN);
	m_MemcardPath[SLOT_B] = new wxFilePickerCtrl(this, ID_MEMCARDPATH_B, wxEmptyString, wxT("Choose a memory card:"),
		wxT("Raw memcards (*.raw)|*.raw"), wxDefaultPosition, wxDefaultSize, wxFLP_USE_TEXTCTRL|wxFLP_OPEN);

	m_MemcardList[SLOT_A] = new CMemcardListCtrl(this, ID_MEMCARDLIST_A, wxDefaultPosition, wxSize(350,400),
		wxLC_REPORT | wxSUNKEN_BORDER | wxLC_ALIGN_LEFT | wxLC_SINGLE_SEL);
	m_MemcardList[SLOT_B] = new CMemcardListCtrl(this, ID_MEMCARDLIST_B, wxDefaultPosition, wxSize(350,400),
		wxLC_REPORT | wxSUNKEN_BORDER | wxLC_ALIGN_LEFT | wxLC_SINGLE_SEL);

	m_MemcardList[SLOT_A]->AssignImageList(new wxImageList(96,32),wxIMAGE_LIST_SMALL);
	m_MemcardList[SLOT_B]->AssignImageList(new wxImageList(96,32),wxIMAGE_LIST_SMALL);

	t_Status[SLOT_A] = new wxStaticText(this, 0, wxEmptyString, wxDefaultPosition,wxDefaultSize, 0, wxEmptyString);
	t_Status[SLOT_B] = new wxStaticText(this, 0, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, wxEmptyString);

	// buttons
	m_CopyFrom[SLOT_A] = new wxButton(this, ID_COPYFROM_A, wxT("->Copy->"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
	m_CopyFrom[SLOT_B] = new wxButton(this, ID_COPYFROM_B, wxT("<-Copy<-"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
	
	m_FixChecksum[SLOT_A] = new wxButton(this, ID_FIXCHECKSUM_A, wxT("<-Fix Checksum"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
	m_FixChecksum[SLOT_B] = new wxButton(this, ID_FIXCHECKSUM_B, wxT("Fix Checksum->"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);

	m_SaveImport[SLOT_A] = new wxButton(this, ID_SAVEIMPORT_A, wxT("<-Import GCI"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
	m_SaveImport[SLOT_B] = new wxButton(this, ID_SAVEIMPORT_B, wxT("Import GCI->"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
	
	m_SaveExport[SLOT_A] = new wxButton(this, ID_SAVEEXPORT_A, wxT("<-Export GCI"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
	m_SaveExport[SLOT_B] = new wxButton(this, ID_SAVEEXPORT_B, wxT("Export GCI->"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);

	m_ConvertToGci = new wxButton(this, ID_CONVERTTOGCI, wxT("Convert to GCI"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);

	m_Delete[SLOT_A] = new wxButton(this, ID_DELETE_A, wxT("<-Delete"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
	m_Delete[SLOT_B] = new wxButton(this, ID_DELETE_B, wxT("Delete->"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);

	m_PrevPage[SLOT_A] = new wxButton(this, ID_PREVPAGE_A, wxT("Prev Page"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
	m_PrevPage[SLOT_B] = new wxButton(this, ID_PREVPAGE_B, wxT("Prev Page"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);

	m_NextPage[SLOT_A] = new wxButton(this, ID_NEXTPAGE_A, wxT("Next Page"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
	m_NextPage[SLOT_B] = new wxButton(this, ID_NEXTPAGE_B, wxT("Next Page"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
	
	// Sizers that double as wxStaticBoxes
	sMemcard_A = new wxStaticBoxSizer(wxVERTICAL, this, wxT("Memory Card A"));
	sMemcard_B = new wxStaticBoxSizer(wxVERTICAL, this, wxT("Memory Card B"));

	// mmmm sizer goodness
	wxBoxSizer* sButtons;
	sButtons = new wxBoxSizer(wxVERTICAL);
	sButtons->AddStretchSpacer(2);
	sButtons->Add(m_CopyFrom[SLOT_B], 0, wxEXPAND, 5);
	sButtons->Add(m_CopyFrom[SLOT_A], 0, wxEXPAND, 5);
	sButtons->AddStretchSpacer(1);
	sButtons->Add(m_FixChecksum[SLOT_A], 0, wxEXPAND, 5);
	sButtons->Add(m_FixChecksum[SLOT_B], 0, wxEXPAND, 5);
	sButtons->AddStretchSpacer(1);
	sButtons->Add(m_SaveImport[SLOT_A], 0, wxEXPAND, 5);
	sButtons->Add(m_SaveExport[SLOT_A], 0, wxEXPAND, 5);
	sButtons->AddStretchSpacer(1);
	sButtons->Add(m_ConvertToGci, 0, wxEXPAND, 5);
	sButtons->AddStretchSpacer(1);
	sButtons->Add(m_SaveImport[SLOT_B], 0, wxEXPAND, 5);
	sButtons->Add(m_SaveExport[SLOT_B], 0, wxEXPAND, 5);
	sButtons->AddStretchSpacer(1);
	sButtons->Add(m_Delete[SLOT_A], 0, wxEXPAND, 5);
	sButtons->Add(m_Delete[SLOT_B], 0, wxEXPAND, 5);
	sButtons->AddStretchSpacer(1);

	sPages_A = new wxBoxSizer(wxHORIZONTAL);
	sPages_B = new wxBoxSizer(wxHORIZONTAL);
	
	sPages_A->Add(m_PrevPage[SLOT_A], 0, wxEXPAND|wxALL, 1);
	sPages_A->Add(t_Status[SLOT_A],0, wxEXPAND|wxALL, 5);
	sPages_A->Add(0, 0, 1, wxEXPAND|wxALL, 0);
	sPages_A->Add(m_NextPage[SLOT_A], 0, wxEXPAND|wxALL, 1);
	sPages_B->Add(m_PrevPage[SLOT_B], 0, wxEXPAND|wxALL, 1);
	sPages_B->Add(t_Status[SLOT_B], 0, wxEXPAND|wxALL, 5);
	sPages_B->Add(0, 0, 1, wxEXPAND|wxALL, 0);
	sPages_B->Add(m_NextPage[SLOT_B], 0, wxEXPAND|wxALL, 1);

	sMemcard_A->Add(m_MemcardPath[SLOT_A], 0, wxEXPAND|wxALL, 5);
	sMemcard_A->Add(m_MemcardList[SLOT_A], 1, wxEXPAND|wxALL, 5);
	sMemcard_A->Add(sPages_A, 0, wxEXPAND|wxALL, 1);
	sMemcard_B->Add(m_MemcardPath[SLOT_B], 0, wxEXPAND|wxALL, 5);
	sMemcard_B->Add(m_MemcardList[SLOT_B], 1, wxEXPAND|wxALL, 5);
	sMemcard_B->Add(sPages_B, 0, wxEXPAND|wxALL, 1);

	sMain = new wxBoxSizer(wxHORIZONTAL);
	sMain->Add(sMemcard_A, 1, wxEXPAND|wxALL, 5);
	sMain->Add(sButtons, 0, wxEXPAND, 0);
	sMain->Add(sMemcard_B, 1, wxEXPAND|wxALL, 5);

	this->SetSizer(sMain);
	sMain->SetSizeHints(this);
	Fit();

	m_PrevPage[SLOT_A]->Disable();
	m_NextPage[SLOT_A]->Disable();
	m_PrevPage[SLOT_B]->Disable();
	m_NextPage[SLOT_B]->Disable();
	m_CopyFrom[SLOT_A]->Disable();
	m_CopyFrom[SLOT_B]->Disable();
	m_FixChecksum[SLOT_A]->Disable();
	m_FixChecksum[SLOT_B]->Disable();
	m_SaveImport[SLOT_A]->Disable();
	m_SaveExport[SLOT_A]->Disable();
	m_SaveImport[SLOT_B]->Disable();
	m_SaveExport[SLOT_B]->Disable();
	m_Delete[SLOT_A]->Disable();
	m_Delete[SLOT_B]->Disable();

}

void CMemcardManager::OnClose(wxCloseEvent& WXUNUSED (event))
{
	Destroy();
}

void CMemcardManager::OnPathChange(wxFileDirPickerEvent& event)
{
	int slot = SLOT_B;
	int slot2 = SLOT_A;
	switch (event.GetId())
	{
	case ID_MEMCARDPATH_A:
		slot = SLOT_A;
		slot2 = SLOT_B;
	case ID_MEMCARDPATH_B:
		page[slot] = FIRSTPAGE;
		if (m_MemcardList[slot]->usePages && m_PrevPage[slot]->IsEnabled())
		{
			m_PrevPage[slot]->Disable();
			m_MemcardList[slot]->prevPage = false;
		}
		if (!strcasecmp(m_MemcardPath[slot2]->GetPath().mb_str(), m_MemcardPath[slot]->GetPath().mb_str()))
		{
			PanicAlert(E_ALREADYOPENED);
		}
		else if (ReloadMemcard(event.GetPath().mb_str(), slot))
		{
			m_MemcardList[slot2]->twoCardsLoaded = true;
			m_FixChecksum[slot]->Enable();
			m_SaveImport[slot]->Enable();
			m_SaveExport[slot]->Enable();
			m_Delete[slot]->Enable();
			break;
		}
		if (memoryCard[slot])
		{
			delete memoryCard[slot];
			memoryCard[slot] = NULL;
		}
		m_MemcardList[slot2]->twoCardsLoaded = false;
		m_MemcardPath[slot]->SetPath(wxEmptyString);
		m_MemcardList[slot]->ClearAll();
		t_Status[slot]->SetLabel(wxEmptyString);
		m_FixChecksum[slot]->Disable();
		m_SaveImport[slot]->Disable();
		m_SaveExport[slot]->Disable();
		m_Delete[slot]->Disable();
		if (m_MemcardList[slot]->usePages)
		{
			m_PrevPage[slot]->Disable();
			m_NextPage[slot]->Disable();
		}
		break;
	}
	if (m_Delete[slot]->IsEnabled() && m_Delete[slot2]->IsEnabled())
	{
		m_CopyFrom[SLOT_A]->Enable();
		m_CopyFrom[SLOT_B]->Enable();
	}
	else
	{
		m_CopyFrom[SLOT_A]->Disable();
		m_CopyFrom[SLOT_B]->Disable();
	}
}

void CMemcardManager::OnPageChange(wxCommandEvent& event)
{
	int slot = SLOT_B;
	switch (event.GetId())
	{
	case ID_NEXTPAGE_A:
		slot = SLOT_A;
	case ID_NEXTPAGE_B:
		if (!m_PrevPage[slot]->IsEnabled())
		{
			m_PrevPage[slot]->Enable();
			m_MemcardList[slot]->prevPage = true;
		}
		page[slot]++;
		if (page[slot] == maxPages)
		{
			m_NextPage[slot]->Disable();
			m_MemcardList[slot]->nextPage = false;
		}
		ReloadMemcard(m_MemcardPath[slot]->GetPath().mb_str(), slot);
		break;
	case ID_PREVPAGE_A:
		slot = SLOT_A;
	case ID_PREVPAGE_B:
		if (!m_NextPage[slot]->IsEnabled())
		{
			m_NextPage[slot]->Enable();
			m_MemcardList[slot]->nextPage = true;
		}
		page[slot]--;
		if (!page[slot])
		{
			m_PrevPage[slot]->Disable();
			m_MemcardList[slot]->prevPage = false;
		}
		ReloadMemcard(m_MemcardPath[slot]->GetPath().mb_str(), slot);
		break;
	}
}

void CMemcardManager::OnMenuChange(wxCommandEvent& event)
{
	if (event.GetId() == ID_USEPAGES)
	{
		m_MemcardList[SLOT_A]->usePages = !m_MemcardList[SLOT_A]->usePages;
		m_MemcardList[SLOT_B]->usePages = !m_MemcardList[SLOT_B]->usePages;
		if (!m_MemcardList[SLOT_A]->usePages)
		{
			m_PrevPage[SLOT_A]->Disable();
			m_PrevPage[SLOT_B]->Disable();
			m_NextPage[SLOT_A]->Disable();
			m_NextPage[SLOT_B]->Disable();
			page[SLOT_A] = page[SLOT_B] = FIRSTPAGE;
		}
	}
	else
	{
		m_MemcardList[SLOT_A]->column[event.GetId()] = !m_MemcardList[SLOT_A]->column[event.GetId()];
		m_MemcardList[SLOT_B]->column[event.GetId()] = !m_MemcardList[SLOT_B]->column[event.GetId()];
	}
	if (memoryCard[SLOT_A])	ReloadMemcard(m_MemcardPath[SLOT_A]->GetPath().mb_str(), SLOT_A);
	if (memoryCard[SLOT_B])	ReloadMemcard(m_MemcardPath[SLOT_B]->GetPath().mb_str(), SLOT_B);

}
bool CMemcardManager::CopyDeleteSwitch(u32 error, int slot)
{
	switch (error)
	{
	case GCS:
		SuccessAlert("File converted to .gci");
		break;
	case SUCCESS:
		if (slot != -1)
		{
			memoryCard[slot]->FixChecksums();
			if (!memoryCard[slot]->Save()) PanicAlert(E_SAVEFAILED);
			page[slot] = FIRSTPAGE;
			ReloadMemcard(m_MemcardPath[slot]->GetPath().mb_str(), slot);
		}
		break;
	case NOMEMCARD:
		PanicAlert(E_NOMEMCARD);
		break;
	case OPENFAIL:
		PanicAlert(E_OPENFAIL);
		break;
	case OUTOFBLOCKS:
		if (slot == -1)
		{
			PanicAlert(E_UNK);
			break;
		}
		PanicAlert(wxString::Format(wxT(E_OUTOFBLOCKS), memoryCard[slot]->GetFreeBlocks()));
		break;
	case OUTOFDIRENTRIES:
		PanicAlert(E_OUTOFDIRENTRIES);
		break;
	case LENGTHFAIL:
		PanicAlert(E_LENGTHFAIL);
		break;
	case INVALIDFILESIZE:
		PanicAlert(E_INVALIDFILESIZE);
		break;
	case TITLEPRESENT:
		PanicAlert(E_TITLEPRESENT);
		break;
	case SAVFAIL:
		PanicAlert(E_SAVFAIL);
		break;
	case GCSFAIL:
		PanicAlert(E_GCSFAIL);
		break;
	case FAIL:
		if (slot == -1) return false;
		PanicAlert(E_INVALID);
		break;
	case WRITEFAIL:
		PanicAlert(E_SAVEFAILED);
		break;
	default:
		PanicAlert(E_UNK);
		break;
	}
	return true;
}

void CMemcardManager::CopyDeleteClick(wxCommandEvent& event)
{
	
	int index_A = m_MemcardList[SLOT_A]->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	int index_B = m_MemcardList[SLOT_B]->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	int slot = SLOT_B;
	int slot2 = SLOT_A;
	int index = index_B;
	std::string fileName2("");

	if (index_A != wxNOT_FOUND && page[SLOT_A]) index_A += itemsPerPage * page[SLOT_A];
	if (index_B != wxNOT_FOUND && page[SLOT_B]) index_B += itemsPerPage * page[SLOT_B];

	switch (event.GetId())
	{
	case ID_COPYFROM_B:
		slot = SLOT_A;
		slot2 = SLOT_B;
	case ID_COPYFROM_A:
		index = slot2 ? index_B : index_A;
		if ((index != wxNOT_FOUND))
		{
			CopyDeleteSwitch(memoryCard[slot]->CopyFrom(*memoryCard[slot2], index), slot);
		}
		break;
	case ID_FIXCHECKSUM_A:
		slot = SLOT_A;
	case ID_FIXCHECKSUM_B:
		if (memoryCard[slot]->FixChecksums() && memoryCard[slot]->Save())
		{
			SuccessAlert("The checksum was successfully fixed");
		}
		else PanicAlert(E_SAVEFAILED);
		break; 
	case ID_CONVERTTOGCI:
		fileName2 = "convert";
	case ID_SAVEIMPORT_A:
		slot = SLOT_A;
	case ID_SAVEIMPORT_B:
	{
		wxString temp = wxFileSelector(_T("Select a save file to import"),
			wxEmptyString, wxEmptyString, wxEmptyString,wxString::Format
			(
				_T("Gamecube save files(*.gci,*.gcs,*.sav)|*.gci;*.gcs;*.sav|"
				"Native GCI files (*.gci)|*.gci|"
				"MadCatz Gameshark files(*.gcs)|*.gcs|"
				"Datel MaxDrive/Pro files(*.sav)|*.sav"),
				wxFileSelectorDefaultWildcardStr,
				wxFileSelectorDefaultWildcardStr
			),
			wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		const char * fileName = temp.ToAscii();
		if (!temp.empty() && !fileName2.empty())
		{
			wxString temp2 = wxFileSelector(_T("Save GCI as.."),
				wxEmptyString, wxEmptyString, _T(".gci"), wxString::Format
				(
					_T("GCI File(*.gci)|*.gci"),
					wxFileSelectorDefaultWildcardStr,
					wxFileSelectorDefaultWildcardStr
				),
				wxFD_OVERWRITE_PROMPT|wxFD_SAVE);
			if (temp2.empty()) break;
			fileName2 = temp2.mb_str();
		}
		if (temp.length() > 0)
		{
			CopyDeleteSwitch(memoryCard[slot]->ImportGci(fileName, fileName2), slot);
		}
	}
	break;
	case ID_SAVEEXPORT_A:
		slot=SLOT_A;
		index = index_A;
	case ID_SAVEEXPORT_B:
		if (index != wxNOT_FOUND)
		{
			wxString temp = wxFileSelector(_T("Save GCI as.."),
					wxEmptyString, wxEmptyString, _T(".gci"), wxString::Format
					(
							_T("GCI File(*.gci)|*.gci"),
							wxFileSelectorDefaultWildcardStr,
							wxFileSelectorDefaultWildcardStr
					),
					wxFD_OVERWRITE_PROMPT|wxFD_SAVE);
			const char * fileName = temp.ToAscii();
			if (temp.length() > 0)
			{
				if (!CopyDeleteSwitch(memoryCard[slot]->ExportGci(index, fileName), -1))
					File::Delete(temp.ToAscii());
			}
		}
		break;
	case ID_DELETE_A:
		slot=SLOT_A;
		index = index_A;
	case ID_DELETE_B:
		if (index != wxNOT_FOUND)
		{
			CopyDeleteSwitch(memoryCard[slot]->RemoveFile(index), slot);
		}
		break;
	}
}

bool CMemcardManager::ReloadMemcard(const char *fileName, int card)
{	
	wxString wxBlock;
	wxString wxFirstBlock;
	wxString wxLabel;
	int j;

	if (memoryCard[card]) delete memoryCard[card];

	// TODO: add error checking and animate icons
	memoryCard[card] = new GCMemcard(fileName);

	if (memoryCard[card]->fail) return false;

	m_MemcardList[card]->Hide();
	m_MemcardList[card]->ClearAll();

	m_MemcardList[card]->InsertColumn(COLUMN_BANNER, _T("Banner"));
	m_MemcardList[card]->InsertColumn(COLUMN_TITLE, _T("Title"));
	m_MemcardList[card]->InsertColumn(COLUMN_COMMENT, _T("Comment"));
	m_MemcardList[card]->InsertColumn(COLUMN_ICON, _T("Icon"));
	m_MemcardList[card]->InsertColumn(COLUMN_BLOCKS, _T("Blocks"));
	m_MemcardList[card]->InsertColumn(COLUMN_FIRSTBLOCK, _T("First Block"));

	wxImageList *list = m_MemcardList[card]->GetImageList(wxIMAGE_LIST_SMALL);
	list->RemoveAll();

	int nFiles = memoryCard[card]->GetNumFiles();
	int *images = new int[nFiles*2];

	for (int i = 0;i < nFiles;i++)
	{
		static u32 pxdata[96*32];
		static u8  animDelay[8];
		static u32 animData[32*32*8];

		int numFrames = memoryCard[card]->ReadAnimRGBA8(i,animData,animDelay);

		if (!memoryCard[card]->ReadBannerRGBA8(i,pxdata))
		{
			memset(pxdata,0,96*32*4);

			if (numFrames>0) // Just use the first one
			{
				u32 *icdata = animData;

				for (int y=0;y<32;y++)
				{
					for (int x=0;x<32;x++)
					{
						pxdata[y*96+x+32] = icdata[y*32+x];//  | 0xFF000000
					}
				}
			}
		}

		wxBitmap map = wxBitmapFromMemoryRGBA((u8*)pxdata,96,32);
		images[i*2] = list->Add(map);

		if (numFrames>0)
		{
			memset(pxdata,0,96*32*4);
			int frames=3;
			if (numFrames<frames) frames=numFrames;
			for (int f=0;f<frames;f++)
			{
				for (int y=0;y<32;y++)
				{
					for (int x=0;x<32;x++)
					{
						pxdata[y*96 + x + 32*f] = animData[f*32*32 + y*32 + x];
					}
				}
			}
			wxBitmap icon = wxBitmapFromMemoryRGBA((u8*)pxdata,96,32);
			images[i*2+1] = list->Add(icon);
		}
	}
	int pagesMax = 128;
	if (m_MemcardList[card]->usePages) pagesMax = (page[card] + 1) * itemsPerPage;
	for (j = page[card] * itemsPerPage;(j < nFiles) && (j < pagesMax);j++)
	{
		char title[32];
		char comment[32];
		u16 blocks;
		u16 firstblock;

		int index = m_MemcardList[card]->InsertItem(j, wxEmptyString);

		m_MemcardList[card]->SetItem(index, COLUMN_BANNER, wxEmptyString);
		if (!memoryCard[card]->GetComment1(j, title)) title[0]=0;
		m_MemcardList[card]->SetItem(index, COLUMN_TITLE, wxString::FromAscii(title));
		if (!memoryCard[card]->GetComment2(j, comment)) comment[0]=0;
		m_MemcardList[card]->SetItem(index, COLUMN_COMMENT, wxString::FromAscii(comment));
		blocks = memoryCard[card]->GetFileSize(j);
		if (blocks == 0xFFFF) blocks = 0;
		wxBlock.Printf(wxT("%10d"), blocks);
		m_MemcardList[card]->SetItem(index,COLUMN_BLOCKS, wxBlock);
		firstblock = memoryCard[card]->GetFirstBlock(j);
		if (firstblock == 0xFFFF) firstblock = 3;	// to make firstblock -1
		wxFirstBlock.Printf(wxT("%15d"), firstblock-4);
		m_MemcardList[card]->SetItem(index,COLUMN_FIRSTBLOCK, wxFirstBlock);
		m_MemcardList[card]->SetItem(index, COLUMN_ICON, wxEmptyString);

		if (images[j] >= 0)
		{
			m_MemcardList[card]->SetItemImage(index, images[j*2]);
			m_MemcardList[card]->SetItemColumnImage(index, COLUMN_ICON, images[j*2+1]);
		}
	}

	if (m_MemcardList[card]->usePages)
	{
		if ((j == nFiles))
		{
			m_NextPage[card]->Disable();
			m_MemcardList[card]->nextPage = false;
		}
		else
		{
			m_NextPage[card]->Enable();
			m_MemcardList[card]->nextPage = true;
		}
	}

	delete[] images;
	// Automatic column width and then show the list
	for (int i = 0; i < NUMBER_OF_COLUMN; i++)
	{
		if (m_MemcardList[card]->column[i])
			m_MemcardList[card]->SetColumnWidth(i, wxLIST_AUTOSIZE);
		else
			m_MemcardList[card]->SetColumnWidth(i, 0);
	}

	m_MemcardList[card]->Show();
	wxLabel.Printf(wxT("%d Free Blocks; %d Free Dir Entries"),
		memoryCard[card]->GetFreeBlocks(), 127 - nFiles);
	t_Status[card]->SetLabel(wxLabel);

	return true;
}

void CMemcardManager::CMemcardListCtrl::OnRightClick(wxMouseEvent& event)
{

	int flags;
	long item = HitTest(event.GetPosition(), flags);

	if (item != wxNOT_FOUND) 
	{
		if (GetItemState(item, wxLIST_STATE_SELECTED) != wxLIST_STATE_SELECTED)
		{
			SetItemState(item, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
		}
		SetItemState(item, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);

		wxMenu popupMenu;
		if (event.GetId() == ID_MEMCARDLIST_A)
		{
			popupMenu.Append(ID_COPYFROM_A, wxT("Copy to Memcard B"));
			popupMenu.Append(ID_DELETE_A, wxT("Delete Save"));
			popupMenu.Append(ID_SAVEIMPORT_A, wxT("Import Save"));
			popupMenu.Append(ID_SAVEEXPORT_A, wxT("Export Save"));
			if (!twoCardsLoaded) 
				popupMenu.FindItem(ID_COPYFROM_A)->Enable(false);
			popupMenu.AppendSeparator();
			popupMenu.Append(ID_FIXCHECKSUM_A, wxT("Fix Checksum"));
			popupMenu.Append(ID_PREVPAGE_A, wxT("Previous Page"));
			popupMenu.Append(ID_NEXTPAGE_A, wxT("Next Page"));
			if (!prevPage || !usePages)
			popupMenu.FindItem(ID_PREVPAGE_A)->Enable(false);
			if (!nextPage || !usePages)
			popupMenu.FindItem(ID_NEXTPAGE_A)->Enable(false);
		}
		else if (event.GetId() == ID_MEMCARDLIST_B)
		{
			popupMenu.Append(ID_COPYFROM_B, wxT("Copy to Memcard A"));
			popupMenu.Append(ID_DELETE_B, wxT("Delete Save"));
			popupMenu.Append(ID_SAVEIMPORT_B, wxT("Import Save"));
			popupMenu.Append(ID_SAVEEXPORT_B, wxT("Export Save"));
			if (!twoCardsLoaded) 
				popupMenu.FindItem(ID_COPYFROM_B)->Enable(false);
			popupMenu.AppendSeparator();
			popupMenu.Append(ID_FIXCHECKSUM_B, wxT("Fix Checksum"));
			popupMenu.Append(ID_PREVPAGE_B, wxT("Previous Page"));
			popupMenu.Append(ID_NEXTPAGE_B, wxT("Next Page"));
			if (!prevPage || !usePages)
			popupMenu.FindItem(ID_PREVPAGE_B)->Enable(false);
			if (!nextPage || !usePages)
			popupMenu.FindItem(ID_NEXTPAGE_B)->Enable(false);
		}
			popupMenu.AppendCheckItem(COLUMN_BANNER, wxT("Show save banner"));
			if(column[COLUMN_BANNER]) popupMenu.FindItem(COLUMN_BANNER)->Check();

			popupMenu.AppendCheckItem(COLUMN_TITLE, wxT("Show save title"));
			if(column[COLUMN_TITLE]) popupMenu.FindItem(COLUMN_TITLE)->Check();

			popupMenu.AppendCheckItem(COLUMN_COMMENT, wxT("Show save comment"));
			if(column[COLUMN_COMMENT]) popupMenu.FindItem(COLUMN_COMMENT)->Check();

			popupMenu.AppendCheckItem(COLUMN_ICON, wxT("Show save icon"));
			if(column[COLUMN_ICON]) popupMenu.FindItem(COLUMN_ICON)->Check();

			popupMenu.AppendCheckItem(COLUMN_BLOCKS, wxT("Show save blocks"));
			if(column[COLUMN_BLOCKS]) popupMenu.FindItem(COLUMN_BLOCKS)->Check();

			popupMenu.AppendCheckItem(COLUMN_FIRSTBLOCK, wxT("Show save first block"));
			if(column[COLUMN_FIRSTBLOCK]) popupMenu.FindItem(COLUMN_FIRSTBLOCK)->Check();

			popupMenu.AppendCheckItem(ID_USEPAGES, wxT("Enable pages"));
			if(usePages) popupMenu.FindItem(ID_USEPAGES)->Check();
			
		PopupMenu(&popupMenu);
	}
}
