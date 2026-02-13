/*
  Copyright (C) 2003 - 2026  Razvan Cojocaru <razvanc@mailbox.org>
  ListDirty() patch contributed by Iulian Dragos
  <dragosiulian@users.sourceforge.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA 02110-1301, USA.
*/

#include <chmhtmlnotebook.h>
#include <chminputstream.h>
#include <chmlistctrl.h>
#include <vector>
#include <wx/aui/auibook.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/listbase.h>
#include <wx/settings.h>
#include <wx/stattext.h>

enum Columns { COL_TITLE };

enum { ID_TOPICS_LIST = 1000, ID_SHOW_BUTTON };

struct TopicsDialog : public wxDialog {
    TopicsDialog(wxWindow* parent, wxWindowID id, const wxString& title, const std::vector<wxString>& urls)
        : wxDialog(parent, id, _("Topics found"))
    {
        auto sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(new wxStaticText(this, wxID_ANY, _("Click on a topic and then on \"Show\"")));
        _listView = new wxListView(this, ID_TOPICS_LIST);
        sizer->Add(_listView, 0, wxEXPAND);

        wxListItem info;
        info.SetId(COL_TITLE);
        info.SetText(_("Title"));
        _listView->InsertColumn(0, info);

        auto buttonSizer  = new wxBoxSizer(wxHORIZONTAL);
        auto showButton   = new wxButton(this, ID_SHOW_BUTTON, _("Show"));
        auto cancelButton = new wxButton(this, wxID_CANCEL);
        buttonSizer->Add(showButton);
        buttonSizer->Add(cancelButton);
        sizer->Add(buttonSizer);

        SetSizerAndFit(sizer);

        int i = 0;
        for ([[maybe_unused]] const auto& topic : urls) {
            _listView->InsertItem(i, title);
            ++i;
        }
    }

    void OnActivated(wxListEvent& event);
    void OnShowButton(wxCommandEvent& event);
    void OnCancelButton(wxCommandEvent& event);

    wxListView* _listView;
    long        _selected;

private:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(TopicsDialog, wxDialog)
EVT_LIST_ITEM_ACTIVATED(ID_TOPICS_LIST, TopicsDialog::OnActivated)
EVT_BUTTON(ID_SHOW_BUTTON, TopicsDialog::OnShowButton)
EVT_BUTTON(wxID_CANCEL, TopicsDialog::OnCancelButton)
END_EVENT_TABLE()

void TopicsDialog::OnShowButton(wxCommandEvent&)
{
    _selected = _listView->GetFirstSelected();
    EndModal(wxOK);
}

void TopicsDialog::OnCancelButton(wxCommandEvent&)
{
    _selected = wxNOT_FOUND;
    EndModal(wxCANCEL);
}

void TopicsDialog::OnActivated(wxListEvent& event)
{
    _selected = event.GetIndex();
    EndModal(wxOK);
}

// Helper

int CompareItemPairs(CHMListPairItem* item1, CHMListPairItem* item2)
{
    return (item1->_title).CmpNoCase(item2->_title);
}

// CHMListCtrl implementation

CHMListCtrl::CHMListCtrl(wxWindow* parent, CHMHtmlNotebook* nbhtml, wxWindowID id)
    : wxListView(parent, id, wxDefaultPosition, wxDefaultSize,
                 wxLC_VIRTUAL | wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL | wxLC_SORT_ASCENDING | wxSUNKEN_BORDER),
      _items(CompareItemPairs), _nbhtml(nbhtml)
{
    constexpr size_t INDEX_HINT_SIZE {2048};

    InsertColumn(0, wxEmptyString);
    SetItemCount(0);

    _items.Alloc(INDEX_HINT_SIZE);
}

CHMListCtrl::~CHMListCtrl()
{
    // Clean the items up.
    ResetItems();
}

void CHMListCtrl::Reset()
{
    ResetItems();
    DeleteAllItems();
    UpdateUI();
}

void CHMListCtrl::UpdateItemCount()
{
    SetItemCount(_items.GetCount());
}

void CHMListCtrl::AddPairItem(const wxString& title, const wxString& url)
{
    _items.Add(new CHMListPairItem(title, url));
}

void CHMListCtrl::AddPairItem(CHMListPairItem* item)
{
    _items.Add(item);
}

void CHMListCtrl::LoadActivated(long item)
{
    if (_items[item]->_urls.size() > 1) {
        TopicsDialog dlg(this, wxID_ANY, _items[item]->_title, _items[item]->_urls);
        if (dlg.ShowModal() == wxOK)
            LoadSelected(item, dlg._selected);
    } else {
        LoadSelected(item);
    }
}

void CHMListCtrl::LoadSelected(long item, long url)
{
    auto chmf = CHMInputStream::GetCache();

    if (chmf) {
        auto fname = _items[item]->_urls[url];

        if (!fname.StartsWith(wxT("file:")))
            fname = wxT("file:") + chmf->ArchiveName() + wxT("#xchm:/") + _items[item]->_urls[url];

        _nbhtml->LoadPageInCurrentView(fname);
    }
}

void CHMListCtrl::UpdateUI()
{
    SetColumnWidth(0, GetClientSize().GetWidth());
}

void CHMListCtrl::FindBestMatch(const wxString& title)
{
    for (size_t i = 0; i < _items.size(); ++i) {
        if (!_items[i]->_title.Left(title.length()).CmpNoCase(title)) {
            EnsureVisible(i);
            Select(i);
            Focus(i);
            break;
        }
    }

    Refresh();
    wxWindow::Update();
}

void CHMListCtrl::ResetItems()
{
    for (auto i = 0UL; i < _items.GetCount(); ++i)
        delete _items[i];

    _items.Empty();
}

void CHMListCtrl::OnSize(wxSizeEvent& event)
{
    UpdateUI();
    event.Skip();
}

wxString CHMListCtrl::OnGetItemText(long item, long column) const
{
    // Is this even possible? item == -1 or item > size - 1?
    if (column != 0 || item == -1L || item > static_cast<long>(_items.GetCount()) - 1)
        return wxT("");

    return _items[item]->_title;
}

BEGIN_EVENT_TABLE(CHMListCtrl, wxListCtrl)
EVT_SIZE(CHMListCtrl::OnSize)
END_EVENT_TABLE()
