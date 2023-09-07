//==============================================================================
PatchBrowser::PatchBrowser (Processor& p) : proc (p)
{
    addAndMakeVisible (authors);
    addAndMakeVisible (tags);
    addAndMakeVisible (presets);

    authors.setColour (juce::ListBox::outlineColourId, juce::Colours::black);
    tags.setColour (juce::ListBox::outlineColourId, juce::Colours::black);
    presets.setColour (juce::ListBox::outlineColourId, juce::Colours::black);

    authors.setOutlineThickness (1);
    tags.setOutlineThickness (1);
    presets.setOutlineThickness (1);

    authors.setMultipleSelectionEnabled (true);
    authors.setClickingTogglesRowSelection (true);
    tags.setMultipleSelectionEnabled (true);
    tags.setClickingTogglesRowSelection (true);

    refresh();
}

void PatchBrowser::updateSelection()
{
    selectedAuthors.clear();
    selectedTags.clear();

    for (int i = 0; i < authors.getNumSelectedRows(); i++)
        selectedAuthors.add (currentAuthors[authors.getSelectedRow (i)]);

    for (int i = 0; i < tags.getNumSelectedRows(); i++)
        selectedTags.add (currentTags[tags.getSelectedRow (i)]);
}

void PatchBrowser::refresh()
{
    currentAuthors.clear();
    currentTags.clear();
    currentPresets.clear();

    for (auto program : proc.getPrograms())
    {
        if (program->author.isNotEmpty())
            currentAuthors.addIfNotAlreadyThere (program->author);

        for (auto t : program->tags)
            if (t.isNotEmpty())
                currentTags.addIfNotAlreadyThere (t);

        if (program->name == "Default")
            continue;

        if (! selectedAuthors.isEmpty())
            if (! selectedAuthors.contains (program->author))
                continue;

        if (! selectedTags.isEmpty())
        {
            bool hasOneTag = false;

            for (auto t : program->tags)
                if (selectedTags.contains (t))
                    hasOneTag = true;

            if (! hasOneTag)
                continue;
        }

        currentPresets.addIfNotAlreadyThere (program->name);
    }

    currentAuthors.sort (true);
    currentTags.sort (true);
    currentPresets.sort (true);

    authors.updateContent();
    tags.updateContent();
    presets.updateContent();

    repaint();
}

void PatchBrowser::resized()
{
    auto r = getLocalBounds().reduced (20);

    int w = (r.getWidth() - 10) / 3;

    authors.setBounds (r.removeFromLeft (w));
    presets.setBounds (r.removeFromRight (w));
    tags.setBounds (r.reduced (5, 0));
}

void PatchBrowser::paint (juce::Graphics& g)
{
    g.fillAll (findColour (PluginLookAndFeel::matte1ColourId));
}

void PatchBrowser::editPreset (int row)
{
    auto p = proc.getProgram (currentPresets[row]);
    if (p == nullptr)
        return;
    
    auto ed = findParentComponentOfClass<ProcessorEditor>();
    
    auto w = std::make_shared<gin::PluginAlertWindow> ("Edit preset:", "", juce::AlertWindow::NoIcon, getParentComponent());
    w->setLookAndFeel (&getLookAndFeel());
    w->addTextEditor ("name", p->name, "Name:");
    w->addTextEditor ("author", p->author, "Author:");
    w->addTextEditor ("tags", p->tags.joinIntoString (" "), "Tags:");

    w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    w->runAsync (*ed, [this, w, row] (int ret)
    {
        w->setVisible (false);
        if (ret == 1)
        {
            auto txt = juce::File::createLegalFileName (w->getTextEditor ("name")->getText());
            auto aut = juce::File::createLegalFileName (w->getTextEditor ("author")->getText());
            auto tag = juce::File::createLegalFileName (w->getTextEditor ("tags")->getText());

            if (proc.hasProgram (txt))
            {
                auto wc = std::make_shared<gin::PluginAlertWindow> ("Preset name '" + txt + "' already in use.", "", juce::AlertWindow::NoIcon, this);
                wc->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
                wc->setLookAndFeel (proc.lf.get());

                wc->runAsync (*getParentComponent(), [wc] (int)
                {
                    wc->setVisible (false);
                });
            }
            else if (txt.isNotEmpty())
            {
                auto& programs = proc.getPrograms();
                programs[row]->deleteFromDir (proc.getProgramDirectory());
                programs[row]->name = txt;
                programs[row]->tags = juce::StringArray::fromTokens (tag, " ", "");
                programs[row]->author = aut;
                programs[row]->saveToDir (proc.getProgramDirectory());

                proc.updateHostDisplay();
                proc.sendChangeMessage();
            }
        }
    });
}

PatchBrowser::AuthorsModel::AuthorsModel (PatchBrowser& o)
    : owner (o)
{
}

int PatchBrowser::AuthorsModel::getNumRows()
{
    return owner.currentAuthors.size();
}

void PatchBrowser::AuthorsModel::selectedRowsChanged (int)
{
    owner.updateSelection();
    owner.refresh();
}

void PatchBrowser::AuthorsModel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    auto rc = juce::Rectangle<int> (0, 0, w, h);

    if (selected)
        g.setColour (owner.findColour (PluginLookAndFeel::accentColourId, true).withAlpha (0.5f));
    else if (row % 2 == 0)
        g.setColour (owner.findColour (PluginLookAndFeel::matte1ColourId, true));
    else
        g.setColour (owner.findColour (PluginLookAndFeel::matte1ColourId, true).overlaidWith (juce::Colours::white.withAlpha (0.02f)));

    g.fillRect (rc);

    g.setColour (owner.findColour (PluginLookAndFeel::whiteColourId, true).withAlpha (0.9f));
    g.setFont (juce::Font (14));
    g.drawText (owner.currentAuthors[row], rc.reduced (4, 0), juce::Justification::centredLeft);
}

PatchBrowser::TagsModel::TagsModel (PatchBrowser& o)
    : owner (o)
{
}

int PatchBrowser::TagsModel::getNumRows()
{
    return owner.currentTags.size();
}

void PatchBrowser::TagsModel::selectedRowsChanged (int)
{
    owner.updateSelection();
    owner.refresh();
}

void PatchBrowser::TagsModel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    auto rc = juce::Rectangle<int> (0, 0, w, h);

    if (selected)
        g.setColour (owner.findColour (PluginLookAndFeel::accentColourId, true).withAlpha (0.5f));
    else if (row % 2 == 0)
        g.setColour (owner.findColour (PluginLookAndFeel::matte1ColourId, true));
    else
        g.setColour (owner.findColour (PluginLookAndFeel::matte1ColourId, true).overlaidWith (juce::Colours::white.withAlpha (0.02f)));

    g.fillRect (rc);

    g.setColour (owner.findColour (PluginLookAndFeel::whiteColourId, true).withAlpha (0.9f));
    g.setFont (juce::Font (14));
    g.drawText (owner.currentTags[row], rc.reduced (4, 0), juce::Justification::centredLeft);
}

PatchBrowser::PresetsModel::PresetsModel (PatchBrowser& o)
    : owner (o)
{
}

int PatchBrowser::PresetsModel::getNumRows()
{
    return owner.currentPresets.size();
}

void PatchBrowser::PresetsModel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    auto rc = juce::Rectangle<int> (0, 0, w, h);

    if (selected)
        g.setColour (owner.findColour (PluginLookAndFeel::accentColourId, true).withAlpha (0.5f));
    else if (row % 2 == 0)
        g.setColour (owner.findColour (PluginLookAndFeel::matte1ColourId, true));
    else
        g.setColour (owner.findColour (PluginLookAndFeel::matte1ColourId, true).overlaidWith (juce::Colours::white.withAlpha (0.02f)));

    g.fillRect (rc);

    g.setColour (owner.findColour (PluginLookAndFeel::whiteColourId, true).withAlpha (0.9f));
    g.setFont (juce::Font (14));
    g.drawText (owner.currentPresets[row], rc.reduced (4, 0), juce::Justification::centredLeft);
}

void PatchBrowser::PresetsModel::listBoxItemDoubleClicked (int row, const juce::MouseEvent&)
{
    owner.proc.setCurrentProgram (owner.currentPresets[row]);
}

void PatchBrowser::PresetsModel::listBoxItemClicked (int row, const juce::MouseEvent& e)
{
    if ( ! e.mouseWasClicked() || ! e.mods.isPopupMenu())
        return;
    
    auto p = owner.proc.getProgram (owner.currentPresets[row]);
    if (p == nullptr)
        return;
    
    auto f = p->getPresetFile (owner.proc.getProgramDirectory());
    
    juce::PopupMenu m;
    m.setLookAndFeel (&owner.getLookAndFeel());
    
   #if JUCE_MAC
    m.addItem ("Reveal in finder", [f] { f.revealToUser(); });
   #elif JUCE_WINDOWS
    m.addItem ("Show in Explorer", [f] { f.revealToUser(); });
   #else
    m.addItem ("Show file", [f] { f.revealToUser(); });
   #endif
    
    m.addItem ("Edit Preset", [this, row] { owner.editPreset (row); });
    
    m.showMenuAsync ({});
}


