#include "LVLExplorerFrame.h"
#include <wx/msgdlg.h>
#include <wx/wfstream.h>
#include <wx/sizer.h>
#include <wx/file.h>
#include <wx/filename.h>


#define ID_MENU_FILE_OPEN 1138
#define ID_MENU_EXIT 1139
#define ID_TREE_VIEW 1140
#define ID_SEARCH 1141

wxBEGIN_EVENT_TABLE(LVLExplorerFrame, wxFrame)
	EVT_MENU(ID_MENU_FILE_OPEN, LVLExplorerFrame::OnMenuOpenFile)
	EVT_MENU(ID_MENU_EXIT, LVLExplorerFrame::OnMenuExit)
	EVT_TREE_SEL_CHANGED(ID_TREE_VIEW, LVLExplorerFrame::OnTreeSelectionChanges)
	EVT_TEXT_ENTER(ID_SEARCH, LVLExplorerFrame::OnSearch)
	EVT_IDLE(LVLExplorerFrame::OnIdle)
wxEND_EVENT_TABLE()

LVLExplorerFrame::LVLExplorerFrame() : wxFrame(
	nullptr,
	wxID_ANY,
	"LVLExplorer", 
	wxDefaultPosition, 
	wxSize(1024, 768))
{
	this->CenterOnScreen();
	SetMinSize(wxSize(600, 400));

	Logger::SetLogfileLevel(ELogType::Warning);
	//m_timer.Bind(wxEVT_TIMER, &LVLExplorerFrame::OnIdle, this);
	//m_timer.Start(100);

	m_menuMain = new wxMenuBar();
	m_fileMenu = new wxMenu();
	m_fileMenu->Append(ID_MENU_FILE_OPEN, "Open");
	m_fileMenu->Append(ID_MENU_EXIT, "Exit");
	m_menuMain->Append(m_fileMenu, "File");
	SetMenuBar(m_menuMain);

	m_panelMain = new wxPanel(this, wxID_ANY);

	m_searchBox = new wxTextCtrl(
		m_panelMain,
		ID_SEARCH,
		"",
		wxDefaultPosition,
		wxDefaultSize,
		wxTE_PROCESS_ENTER
	);

	m_lvlTreeCtrl = new wxTreeCtrl(
		m_panelMain,
		ID_TREE_VIEW,
		wxDefaultPosition,
		wxDefaultSize
	);

	m_textDisplay = new wxTextCtrl(
		m_panelMain,
		wxID_ANY,
		"",
		wxDefaultPosition,
		wxDefaultSize,
		wxTE_MULTILINE
	);
	m_textDisplay->Hide();

	m_imageDisplay = new wxImagePanel(m_panelMain);
	m_imageDisplay->Hide();
	m_imageData = nullptr;
	m_currentContainer = nullptr;

	m_infoText = new wxStaticText(
		m_panelMain,
		wxID_ANY,
		"Chunk Position:\n"
		"Chunk Data Size:\n"
		"Chunk Full Size:",
		wxDefaultPosition,
		wxDefaultSize
	);
	m_infoText->SetMinSize(wxSize(400, 50));
	m_infoText->SetMaxSize(wxSize(400, 50));

	m_sizerHorizontal = new wxBoxSizer(wxHORIZONTAL);
	m_panelMain->SetSizer(m_sizerHorizontal);
	m_panelMain->SetAutoLayout(true);

	m_sizerLeft = new wxBoxSizer(wxVERTICAL);
	m_sizerLeft->Add(m_searchBox, wxSizerFlags().Expand().Border(wxBOTTOM, 10));
	m_sizerLeft->Add(m_lvlTreeCtrl, wxSizerFlags().Expand().Proportion(1));
	m_sizerHorizontal->Add(m_sizerLeft, wxSizerFlags().Expand().Proportion(1).Border(wxALL, 10));

	m_sizerRight = new wxBoxSizer(wxVERTICAL);
	m_sizerRight->Add(m_infoText, wxSizerFlags().Proportion(1).Border(wxTOP, 10));
	m_sizerHorizontal->Add(m_sizerRight, wxSizerFlags().Expand().Proportion(2));

	m_rightHandSideFlags = wxSizerFlags().Expand().Proportion(2).Border(wxTOP | wxRIGHT | wxBOTTOM, 10);
	m_displayStatus = EDisplayStatus::NONE;
	DisplayText();
}

LVLExplorerFrame::~LVLExplorerFrame()
{
	if (m_imageData != nullptr)
	{
		free(m_imageData);
		m_imageData = nullptr;
	}

	DestroyLibContainer();
}

void LVLExplorerFrame::DestroyLibContainer()
{
	if (m_currentContainer == nullptr)
		return;

	Container::Delete(m_currentContainer);
	m_currentContainer = nullptr;
}

void LVLExplorerFrame::DisplayText()
{
	if (m_displayStatus == EDisplayStatus::TEXT)
		return;

	if (m_displayStatus != EDisplayStatus::NONE)
		HideCurrentDisplay();

	m_textDisplay->Show();
	m_sizerRight->Add(m_textDisplay, m_rightHandSideFlags);
	m_sizerRight->Layout();
	m_panelMain->Layout();
	m_displayStatus = EDisplayStatus::TEXT;
}

void LVLExplorerFrame::DisplayImage()
{
	if (m_displayStatus == EDisplayStatus::IMAGE)
		return;

	if (m_displayStatus != EDisplayStatus::NONE)
		HideCurrentDisplay();

	if (m_imageData == nullptr)
	{
		m_imageWidth = 256;
		m_imageHeight = 256;
		size_t size = m_imageWidth * m_imageHeight * 3;
		m_imageData = (uint8_t*)malloc(size);

		for (int i = 0; i < size; ++i)
		{
			// black image
			m_imageData[i] = 0;
		}
		m_imageDisplay->SetImageData(m_imageWidth, m_imageHeight, m_imageData);
	}

	m_imageDisplay->Show();
	m_sizerRight->Add(m_imageDisplay, m_rightHandSideFlags);
	m_sizerRight->Layout();
	m_panelMain->Layout();
	m_displayStatus = EDisplayStatus::IMAGE;
}

void LVLExplorerFrame::HideCurrentDisplay()
{
	switch (m_displayStatus)
	{
		case EDisplayStatus::NONE:
			return;
		case EDisplayStatus::TEXT:
			m_sizerRight->Remove(1);
			m_textDisplay->Hide();
			break;
		case EDisplayStatus::IMAGE:
			m_sizerRight->Remove(1);
			m_imageDisplay->Hide();
			break;
		default:
			wxLogError("Unknown EDisplayStatus %i", (int)m_displayStatus);
			return;
	}
}

void LVLExplorerFrame::OnMenuOpenFile(wxCommandEvent& event)
{
	wxFileDialog dialog(this, "Open Level container file", "", "",
		"SWBF2 Level (*.lvl)|*.lvl|zafbin Animation (*.zafbin)|*.zafbin|zaabin Animation (*.zaabin)|*.zaabin|Sound Bank (*.bnk)|*.bnk|Compiled SWBF2 LUA Script (*.script)|*.script", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (dialog.ShowModal() == wxID_CANCEL)
		return;

	m_lvlTreeCtrl->DeleteAllItems();
	m_treeToChunk.clear();

	if (m_currentContainer != nullptr)
	{
		DestroyLibContainer();
	}
	m_currentContainer = Container::Create();

	wxString path = dialog.GetPath();
	wxFileName fileName(path);
	wxString fileExt = fileName.GetExt().Lower();

	if (fileExt == "lvl" || fileExt == "zafbin" || fileExt == "zaabin" || fileExt == "script")
	{
		m_currentContainer->AddLevel(path.c_str().AsChar());
	}
	else if (fileExt == "bnk")
	{
		m_currentContainer->AddSoundBank(path.c_str().AsChar());
	}
	else
	{
		wxMessageBox(
			wxString::Format("Unknown file extension '%s'!", fileExt),
			"Error",
			wxICON_ERROR);
		return;
	}

	wxASSERT(m_progress == nullptr);
	m_progress = new wxProgressDialog("Loading", "Loading....");
	m_progress->Show();
	
	m_currentContainer->StartLoading();
}

void LVLExplorerFrame::OnMenuExit(wxCommandEvent& event)
{
	Close();
}

void LVLExplorerFrame::OnTreeSelectionChanges(wxTreeEvent& event)
{
	wxTreeItemId item = event.GetItem();
	if (item == m_treeRoot)
	{
		m_infoText->SetLabel(
			"Chunk Position:\n"
			"Chunk Data Size:\n"
			"Chunk Full Size:"
		);
		return;
	}

	auto it = m_treeToChunk.find(item);
	if (it == m_treeToChunk.end())
	{
		wxLogError("Could not find corresponding chunk for tree item ID %i!", item.GetID());
		return;
	}

	const GenericBaseChunk* chunk = it->second;
	m_infoText->SetLabel(wxString::Format(
		"Chunk Position:\t%i\n"
		"Chunk Data Size:\t%i\n"
		"Chunk Full Size:\t%i",
		(uint32_t)chunk->GetPosition(),
		(uint32_t)chunk->GetDataSize(),
		(uint32_t)chunk->GetFullSize()
	));

	const BODY* textureBodyChunk = dynamic_cast<const BODY*>(chunk);

	// Display first mip map texture of first found format if a tex_ chunk is selected
	const tex_* textureChunk = dynamic_cast<const tex_*>(chunk);
	if (textureChunk != nullptr && textureChunk->m_FMTs.Size() > 0 && textureChunk->m_FMTs[0]->p_Face->m_LVLs.Size() > 0)
	{
		textureBodyChunk = textureChunk->m_FMTs[0]->p_Face->m_LVLs[0]->p_Body;
	}

	if (textureBodyChunk != nullptr)
	{
		const uint8_t* data;

		// this delivers R8 G8 B8 A8
		textureBodyChunk->GetImageData(ETextureFormat::R8_G8_B8_A8, m_imageWidth, m_imageHeight, data);

		m_textDisplay->AppendText(wxString::Format("grabbed image data at: %i\n", (int)(size_t)data));
		m_textDisplay->AppendText(wxString::Format("chunk position: %i\n", (int)(size_t)textureBodyChunk->GetPosition()));

		size_t numPixels = m_imageWidth * m_imageHeight;
		if (m_imageData != nullptr)
		{
			free(m_imageData);
		}

		m_imageData = (uint8_t*)malloc(numPixels * 3);
		for (size_t i = 0; i < numPixels; ++i)
		{
			// calc alpha
			const uint8_t alphaColor[3] = { 255, 0, 255 };
			float alpha = data[(i * 4) + 3] / 255.f;
			float transparency = 1.f - alpha;

			m_imageData[(i * 3) + 0] = uint8_t(data[(i * 4) + 0] * alpha + alphaColor[0] * transparency);
			m_imageData[(i * 3) + 1] = uint8_t(data[(i * 4) + 1] * alpha + alphaColor[1] * transparency);
			m_imageData[(i * 3) + 2] = uint8_t(data[(i * 4) + 2] * alpha + alphaColor[2] * transparency);
		}

		m_imageDisplay->SetImageData(m_imageWidth, m_imageHeight, m_imageData);
		DisplayImage();
		//SetSize(888, 666);
		//m_imageDisplay->RefreshRect(m_imageDisplay->GetRect());
		//m_imageDisplay->Refresh();
		//m_imageDisplay->Update();
		//Refresh();
		//Update();

		// stupid workaround...
		{
			static int grow = 1;
			int w, h;
			GetSize(&w, &h);
			SetSize(w + grow, h + grow);
			grow = grow > 0 ? -1 : 1;
		}
	}
	else
	{
		m_textDisplay->Clear();
		m_textDisplay->WriteText(chunk->ToString().Buffer());
		DisplayText();
	}
}

bool LVLExplorerFrame::SearchTree(wxTreeItemId parent, const wxString& search)
{
	wxTreeItemIdValue cookie;
	wxTreeItemId next = m_lvlTreeCtrl->GetFirstChild(parent, cookie);
	bool bFoundInChildren = false;
	while (next.IsOk())
	{
		bFoundInChildren = SearchTree(next, search) || bFoundInChildren;
		next = m_lvlTreeCtrl->GetNextChild(parent, cookie);
	}

	if (search.IsEmpty())
		return false;


	bool bFoundInInfo = false;
	auto it = m_treeToChunk.find(parent);
	if (it != m_treeToChunk.end())
	{
		const GenericBaseChunk* chunk = it->second;
		try
		{
			// sometimes, someone (not LibSWBF2) throws a "string too long" exception (msvcp140d.dll??)
			// I'm too lazy right now to dive depp into that... just ignore that chunk... TODO
			bFoundInInfo = wxString(chunk->ToString().Buffer()).Contains(search);
		}
		catch (std::exception& e)
		{
			//m_textDisplay->AppendText(e.what());
			bFoundInInfo = false;
		}
	}

	bool bFound = bFoundInInfo || m_lvlTreeCtrl->GetItemText(parent).Contains(search);
	if (bFound && !bFoundInChildren)
	{
		m_lvlTreeCtrl->SetItemBackgroundColour(parent, ITEM_COLOR_FOUND_BACKGROUND);
		m_lvlTreeCtrl->SetItemTextColour(parent, bFoundInInfo ? ITEM_COLOR_FOUND_IN_INFO : ITEM_COLOR_FOUND);
		m_lvlTreeCtrl->Collapse(parent);
	}
	else if (bFound && bFoundInChildren)
	{
		m_lvlTreeCtrl->SetItemBackgroundColour(parent, ITEM_COLOR_FOUND_BACKGROUND);
		m_lvlTreeCtrl->SetItemTextColour(parent, bFoundInInfo ? ITEM_COLOR_FOUND_IN_INFO : ITEM_COLOR_FOUND);
		m_lvlTreeCtrl->Expand(parent);
	}
	else if (!bFound && bFoundInChildren)
	{
		m_lvlTreeCtrl->SetItemBackgroundColour(parent, ITEM_COLOR_FOUND_BACKGROUND);
		m_lvlTreeCtrl->SetItemTextColour(parent, ITEM_COLOR_FOUND_CHILDREN);
		m_lvlTreeCtrl->Expand(parent);
	}
	else
	{
		m_lvlTreeCtrl->SetItemBackgroundColour(parent, ITEM_COLOR_BACKGROUND);
		m_lvlTreeCtrl->SetItemTextColour(parent, ITEM_COLOR);
		m_lvlTreeCtrl->Collapse(parent);
	}
	return bFound || bFoundInChildren;
};

void LVLExplorerFrame::OnSearch(wxCommandEvent& event)
{
	if (!m_treeRoot.IsOk())
	{
		return;
	}

	wxString search = event.GetString();
	SearchTree(m_treeRoot, search);
}

void LVLExplorerFrame::ParseChunk(const GenericBaseChunk* chunk, wxTreeItemId parent, size_t childIndex)
{
	wxTreeItemId current = m_lvlTreeCtrl->AppendItem(parent, wxString::Format("[%i] %s", (int)childIndex, chunk->GetHeaderName().Buffer()));
	m_lvlTreeCtrl->SetItemBackgroundColour(current, ITEM_COLOR_BACKGROUND);
	m_lvlTreeCtrl->SetItemTextColour(current, ITEM_COLOR);
	m_treeToChunk.emplace(current, chunk);

	//m_chunkNames += chunk->GetHeaderName().Buffer() + wxString("\n");

	const List<GenericBaseChunk*>& children = chunk->GetChildren();
	for (size_t i = 0; i < children.Size(); ++i)
	{
		ParseChunk(children[i], current, i);
	}
}

void LVLExplorerFrame::OnIdle(wxIdleEvent& event)
{
	LoggerEntry log;
	while (Logger::GetNextLog(log))
	{
		AddLogLine(log.ToString().Buffer());
	}

	if (m_currentContainer != nullptr && m_progress != nullptr)
	{
		if (!m_currentContainer->IsDone())
		{
			int percent = int(m_currentContainer->GetOverallProgress() * 100.0f);
			wxString dspStr = wxString::Format("Loading... %d %%", percent);
			m_progress->Update(percent, dspStr);
		}
		else
		{
			List<SWBF2Handle> handles = m_currentContainer->GetLoadedLevels();
			Level* level = m_currentContainer->GetLevel(handles[0]);

			m_lvlTreeCtrl->Freeze();
			//m_treeRoot = m_lvlTreeCtrl->AddRoot(level->GetLevelName().Buffer());
			m_treeRoot = m_lvlTreeCtrl->AddRoot("root");
			m_lvlTreeCtrl->Thaw();

			m_progress->Update(100, "Parsing...");
			ParseChunk(level->GetChunk(), m_treeRoot);

			m_lvlTreeCtrl->Expand(m_treeRoot);

			//m_progress->Hide();
			delete m_progress;
			m_progress = nullptr;

			m_lvlTreeCtrl->SetFocus();
		}
	}
}

void LVLExplorerFrame::AddLogLine(wxString msg)
{
	if (m_textDisplay != nullptr)
		m_textDisplay->AppendText(msg + "\n");
}