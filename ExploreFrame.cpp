/*
** HLMWadExplorer
**
** Coypright (C) 2015 Tobias Taschner <github@tc84.de>
**
** Licensed under GPL v3 or later
*/

#include "ExploreFrame.h"

#include <wx/app.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/mstream.h>
#include <wx/busyinfo.h>
#include <wx/aboutdlg.h>
#include <wx/textdlg.h>
#include <wx/config.h>
#include <wx/persist.h>
#include <wx/persist/toplevel.h>
#if defined(__WXMSW__)
#include <wx/msw/registry.h>
#endif

#include "TexturePack.h"
#include "TexturePackPanel.h"

class FileDataModel : public wxDataViewVirtualListModel
{
public:
	FileDataModel(WADArchive* archive):
		wxDataViewVirtualListModel(archive->GetEntryCount()),
		m_archive(archive)
	{

	}

	virtual unsigned int GetColumnCount() const
	{
		return 2;
	}

	// return type as reported by wxVariant
	virtual wxString GetColumnType(unsigned int col) const
	{
		return wxString();
	}

	void GetValueByRow(wxVariant &variant, unsigned int row, unsigned int col) const
	{
		const WADArchiveEntry& entry = m_archive->GetEntry(row);
		switch (col)
		{
			case 0:
				variant = entry.GetFileName();
				break;
			case 1:
				variant = wxFileName::GetHumanReadableSize(entry.GetSize());
				break;
		}

	}

	bool SetValueByRow(const wxVariant &variant, unsigned int row, unsigned int col)
	{
		return false;
	}

private:
	WADArchive* m_archive;
};


ExploreFrame::ExploreFrame( wxWindow* parent ):
	BaseExploreFrame( parent ),
	m_archive(NULL)
{
	SetTitle(wxTheApp->GetAppDisplayName());
	m_menubar->Enable(ID_EXTRACT, false);
	m_menubar->Enable(wxID_SAVE, false);
	m_menubar->Enable(wxID_SAVEAS, false);
	m_fileListNameColumn->SetWidth(150);
	m_fileListSizeColumn->SetAlignment(wxALIGN_RIGHT);

	m_fileHistory.UseMenu(m_fileMenu);
	m_fileHistory.Load(*wxConfigBase::Get());

	m_previewBookCtrl->AddPage(new wxPanel(m_previewBookCtrl), "General", true);
	m_previewBookCtrl->AddPage(new ImagePanel(m_previewBookCtrl), "Image", true);
	m_previewBookCtrl->AddPage(new TexturePackPanel(m_previewBookCtrl), "Texture", false);

	Bind(wxEVT_MENU, &ExploreFrame::OnRecentFileClicked, this, wxID_FILE1, wxID_FILE9);

	wxPersistenceManager::Get().RegisterAndRestore(this);
}

void ExploreFrame::OnAboutClicked( wxCommandEvent& event )
{
	wxAboutDialogInfo aboutInfo;
	aboutInfo.SetName(wxTheApp->GetAppDisplayName());
	aboutInfo.SetDescription(_("HLM Wad Extractor"));
	aboutInfo.SetCopyright("(C) 2015");
	aboutInfo.SetWebSite("https://github.com/TcT2k/HLMWadExplorer");
	aboutInfo.AddDeveloper("Tobias Taschner");
	wxAboutBox(aboutInfo);
}

void ExploreFrame::OnWindowClose( wxCloseEvent& event )
{
	if (m_archive && m_archive->IsModified())
	{
		switch (wxMessageBox(_("Do you wan't to save unsaved changes?"), _("Warning"), wxICON_WARNING | wxYES_NO | wxCANCEL | wxNO_DEFAULT, this))
		{
			case wxYES:
			{
				wxBusyInfo busyInfo(_("Writing file..."));
				wxBusyCursor busyCursor;
				m_archive->Save();
				event.Skip();
				break;
			}
			case wxNO:
				event.Skip();
				break;
			default:
				event.Veto();
				break;
		}
	} else
		event.Skip();
}

void ExploreFrame::OnRecentFileClicked(wxCommandEvent& event)
{
	OpenFile(m_fileHistory.GetHistoryFile(event.GetId() - m_fileHistory.GetBaseId()));
}

void ExploreFrame::OnOpenClicked( wxCommandEvent& event )
{
	wxString hlmPath;
#if defined(__WXOSX__)
	hlmPath = wxFileName::GetHomeDir() + "/Library/Application Support/Steam/steamapps/common/Hotline Miami 2/HotlineMiami2.app/Contents/Resources/";
#elif defined(__WXMSW__)
	wxRegKey regKey(wxRegKey::HKCU, "Software\\Valve\\Steam");
	if (regKey.Open(wxRegKey::Read))
	{
		regKey.QueryValue("SteamPath", hlmPath);

		hlmPath += "\\SteamApps\\Common\\Hotline Miami 2";
	}
#endif
	
	wxFileDialog fileDlg(this, _("Select WAD file"), hlmPath, wxString(), "*.wad", wxFD_DEFAULT_STYLE | wxFD_FILE_MUST_EXIST);

	if (fileDlg.ShowModal() == wxID_OK)
	{
		OpenFile(fileDlg.GetPath());
	}
}

void ExploreFrame::OpenFile(const wxString& filename)
{
	m_archive = new WADArchive(filename);
	wxObjectDataPtr<FileDataModel> model(new FileDataModel(m_archive.get()));
	m_fileListCtrl->AssociateModel(model.get());
	m_menubar->Enable(ID_EXTRACT, true);
	m_menubar->Enable(wxID_ADD, true);
	m_menubar->Enable(wxID_SAVE, true);
	m_menubar->Enable(wxID_SAVEAS, true);
#if defined(__WXOSX__)
	OSXSetModified(false);
#endif
	
	wxFileName inputFN(filename);
	SetTitle(wxString::Format("%s - %s", wxTheApp->GetAppDisplayName(), inputFN.GetName()));

	m_fileHistory.AddFileToHistory(filename);
	m_fileHistory.Save(*wxConfigBase::Get());
}

void ExploreFrame::OnSaveClicked( wxCommandEvent& event )
{
	if (wxMessageBox(_("Are you sure you want to overwrite the WAD?"), _("Warning"), wxICON_WARNING | wxYES_NO | wxNO_DEFAULT, this) == wxYES)
	{
		wxBusyInfo busyInfo(_("Writing file..."));
		wxBusyCursor busyCursor;
		m_archive->Save();
#if defined(__WXOSX__)
		OSXSetModified(false);
#endif
	}
}

void ExploreFrame::OnSaveAsClicked( wxCommandEvent& event )
{
	wxFileName saveAsFN(m_archive->GetFileName());
	
	wxFileDialog fileDlg(this, _("Select destination filename"), saveAsFN.GetPath(), saveAsFN.GetFullName(), "*.wad", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fileDlg.ShowModal() == wxID_OK)
	{
		wxBusyInfo busyInfo(_("Writing file..."));
		wxBusyCursor busyCursor;
		m_archive->Save(fileDlg.GetPath());
		OpenFile(fileDlg.GetPath());
	}
}

const WADArchiveEntry& ExploreFrame::GetSelectedEntry() const
{
	size_t index = (size_t) m_fileListCtrl->GetSelection().GetID() - 1;
	return m_archive->GetEntry(index);
}

void ExploreFrame::OnExtractClicked( wxCommandEvent& event )
{
	if (m_fileListCtrl->GetSelectedItemsCount() == 0)
	{
		wxLogError(_("Select file(s) to extract"));
		return;
	}

	wxString extractFolder = wxStandardPaths::Get().GetDocumentsDir();
	if (m_fileListCtrl->GetSelectedItemsCount() > 1)
	{
		wxDirDialog dirDlg(this, _("Select folder to extract files to"), extractFolder, wxDD_DEFAULT_STYLE);
		if (dirDlg.ShowModal() == wxID_OK)
		{
			wxBusyInfo busyInfo(_("Extracting data..."));
			wxBusyCursor busyCursor;

			wxDataViewItemArray selectedItems;
			m_fileListCtrl->GetSelections(selectedItems);

			for (auto it = selectedItems.begin(); it != selectedItems.end(); ++it)
			{
				const WADArchiveEntry& entry = m_archive->GetEntry((size_t) it->GetID() - 1);
				wxFileName targetFileName(entry.GetFileName(), wxPATH_UNIX);
				targetFileName.Normalize(wxPATH_NORM_ALL, dirDlg.GetPath());
				wxLogDebug("Extracting to: %s", targetFileName.GetFullPath());

				if (!targetFileName.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
					break;
				m_archive->Extract(entry, targetFileName.GetFullPath());
			}
		}
		wxLogInfo(_("Files extracted to %s"), dirDlg.GetPath());
	} else {
		const WADArchiveEntry& entry = GetSelectedEntry();

		wxFileName fn(entry.GetFileName(), wxPATH_UNIX);

		wxString fileName = fn.GetFullName();
		wxFileDialog fileDlg(this, _("Select target for file extraction"), extractFolder, fileName, 
			wxFileSelectorDefaultWildcardStr, wxFD_OVERWRITE_PROMPT | wxFD_SAVE);
		if (fileDlg.ShowModal() == wxID_OK)
		{
			wxBusyInfo busyInfo(_("Extracting data..."));
			wxBusyCursor busyCursor;

			m_archive->Extract(entry, fileDlg.GetPath());
		}

		wxLogInfo(_("File extracted to %s"), fileDlg.GetPath());
	}
}

void ExploreFrame::OnReplaceClicked( wxCommandEvent& event )
{
	const WADArchiveEntry& entry = GetSelectedEntry();
	wxFileName entryFN(entry.GetFileName());
	
	wxFileDialog fileDlg(this, wxString::Format(_("Select file to replace %s"), entry.GetFileName()), wxString(),
						 entryFN.GetFullName(), "*." + entryFN.GetExt(), wxFD_DEFAULT_STYLE  | wxFD_FILE_MUST_EXIST);
	if (fileDlg.ShowModal() == wxID_OK)
	{
		size_t index = (size_t) m_fileListCtrl->GetSelection().GetID() - 1;
		m_archive->Replace(index, fileDlg.GetPath());
		static_cast<FileDataModel*>(m_fileListCtrl->GetModel())->RowChanged(index);
		wxDataViewEvent evt(wxEVT_DATAVIEW_SELECTION_CHANGED);
		OnFileListSelectionChanged(evt);
		
#if defined(__WXOSX__)
		OSXSetModified(true);
#endif
	}
}

void ExploreFrame::OnAddClicked( wxCommandEvent& event )
{
	wxFileDialog fileDlg(this, _("Select file to add"), wxString(), wxString(), "*.*", wxFD_DEFAULT_STYLE | wxFD_FILE_MUST_EXIST);
	if (fileDlg.ShowModal() == wxID_OK)
	{
		wxFileName newFN(fileDlg.GetPath());
		wxString newEntryName = newFN.GetFullName();
		
		wxTextEntryDialog dlg(this, _("Enter filename in WAD"), _("Add resource"), newEntryName);
		if (dlg.ShowModal() == wxID_OK)
		{
			newEntryName = dlg.GetValue();
			m_archive->Add(WADArchiveEntry(newEntryName, fileDlg.GetPath()));
			static_cast<FileDataModel*>(m_fileListCtrl->GetModel())->RowAppended();
			
#if defined(__WXOSX__)
			OSXSetModified(true);
#endif
		}
	}
}

void ExploreFrame::OnDeleteClicked( wxCommandEvent& event )
{
	const WADArchiveEntry& entry = GetSelectedEntry();
	if (wxMessageBox(wxString::Format(_("Are you sure you want to delete %s?"), entry.GetFileName()),
									  _("Warning"), wxICON_WARNING | wxYES_NO | wxNO_DEFAULT, this) == wxYES)
	{
		size_t index = (size_t) m_fileListCtrl->GetSelection().GetID() - 1;
		m_archive->Remove(index);
		static_cast<FileDataModel*>(m_fileListCtrl->GetModel())->RowDeleted(index);
		wxDataViewEvent evt(wxEVT_DATAVIEW_SELECTION_CHANGED);
		OnFileListSelectionChanged(evt);
		
#if defined(__WXOSX__)
		OSXSetModified(true);
#endif
	}
}

void ExploreFrame::OnQuitClicked( wxCommandEvent& event )
{
	Close();
}

void ExploreFrame::OnFileListSelectionChanged( wxDataViewEvent& event )
{
	m_menubar->Enable(ID_REPLACE, m_fileListCtrl->GetSelectedItemsCount() > 0);
	m_menubar->Enable(ID_EXTRACT, m_fileListCtrl->GetSelectedItemsCount() > 0);
	m_menubar->Enable(wxID_DELETE, m_fileListCtrl->GetSelectedItemsCount() > 0);
	
	if (m_fileListCtrl->GetSelectedItemsCount() != 1)
		return;
		
	size_t index = (size_t)m_fileListCtrl->GetSelection().GetID() - 1;
	const WADArchiveEntry& entry = m_archive->GetEntry(index);
	wxLogDebug("Selected: %s", entry.GetFileName());

	wxFileName fn(entry.GetFileName(), wxPATH_UNIX);
	wxString fileExt = fn.GetExt();

	if (fileExt.IsSameAs("png", false) ||
		fileExt.IsSameAs("jpg", false))
	{
		m_previewBookCtrl->ChangeSelection(1);

		ImagePanel* imgPanel = (ImagePanel*) m_previewBookCtrl->GetCurrentPage();
		imgPanel->m_previewBitmap->SetBitmap(m_archive->ExtractBitmap(entry));
		imgPanel->Layout();
	}
	else if (fileExt.IsSameAs("meta", false))
	{
		m_previewBookCtrl->ChangeSelection(2);

		// Load texture pack
		wxMemoryOutputStream oStr;
		m_archive->Extract(entry, oStr);
		wxStreamBuffer* buffer = oStr.GetOutputStreamBuffer();
		wxMemoryInputStream iStr(buffer->GetBufferStart(), buffer->GetBufferSize());

		wxFileName imageFN(entry.GetFileName(), wxPATH_UNIX);
		imageFN.SetExt("png");
		WADArchiveEntry imgEntry = m_archive->GetEntry(imageFN.GetFullPath(wxPATH_UNIX));

		TexturePackPanel* texPanel = (TexturePackPanel*)m_previewBookCtrl->GetCurrentPage();
		texPanel->LoadTexture(iStr, m_archive->ExtractBitmap(imgEntry));
	}
	else
		m_previewBookCtrl->ChangeSelection(0);
}

void ExploreFrame::OnFileListDoubleClick( wxMouseEvent& event )
{
// TODO: Implement OnFileListDoubleClick
}
