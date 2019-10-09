#include "mainlayout.h"
#include <et/app/application.h>
#include <et/core/tools.h>

namespace et
{

MainLayout::MainLayout(RenderContext* rc)
{
	ObjectsCache localCache;
	s2d::CharacterGenerator::Pointer defaultCharacters = s2d::CharacterGenerator::Pointer::create(rc, "Tahoma", "Tahoma");
	_mainFont = s2d::Font::Pointer::create(defaultCharacters);
	_mainFont->loadFromFile(rc, application().resolveFileName("engine_ext_data/fonts/tahoma.font"), localCache);

	vec4 defaultBackgroundColor = vec4(1.0f, 0.5f, 0.25f, 1.0f);
	float defaultFontSize = 16.0f;
	float defaultButtonSize = 0.15f;
	float defaultButtonOffset = 0.16f;
	float defaultButtonGap = 0.005f;

	s2d::Button::Pointer btnOpen = s2d::Button::Pointer::create("Open", _mainFont, defaultFontSize, this);
	btnOpen->setFontSmoothing(2.25f);
	btnOpen->setAutolayoutRelativeToParent(vec2(defaultButtonGap), vec2(defaultButtonSize, 0.05f), vec2(0.0f));
	btnOpen->setBackgroundColor(defaultBackgroundColor);
	btnOpen->setTextPressedColor(vec4(1.0f));
	btnOpen->clicked.connect(this, &MainLayout::onBtnOpenClick);

	s2d::Button::Pointer btnExport = s2d::Button::Pointer::create("Export", _mainFont, defaultFontSize, this);
	btnExport->setFontSmoothing(2.25f);
	btnExport->setAutolayoutRelativeToParent(vec2(1.0f - defaultButtonGap, defaultButtonGap),
		vec2(defaultButtonSize, 0.05f), vec2(1.0f, 0.0f));
	btnExport->setBackgroundColor(defaultBackgroundColor);
	btnExport->setTextPressedColor(vec4(1.0f));
	btnExport->clicked.connect(this, &MainLayout::onBtnSaveClick);

	_btnWireframe = s2d::Button::Pointer::create("Wireframe", _mainFont, defaultFontSize, this);
	_btnWireframe->setLocationInParent(s2d::Location_BottomLeft, vec2(defaultButtonOffset, defaultButtonOffset));
	_btnWireframe->setBackgroundColor(defaultBackgroundColor);
	_btnWireframe->setTextPressedColor(vec4(1.0f));
	_btnWireframe->setType(s2d::Button::Type_CheckButton);
	_btnWireframe->setSelected(false);
	_btnWireframe->setFontSmoothing(2.25f);
	_btnWireframe->clicked.connect([this](s2d::Button*) {
		wireframeChanged.invokeInMainRunLoop(_btnWireframe->selected());
	});

	_labStatus = s2d::Label::Pointer::create("Ready", _mainFont, defaultFontSize, this);
	_labStatus->setAutolayoutRelativeToParent(vec2(1.0f - defaultButtonGap), vec2(0.0f), vec2(1.0f));
	_labStatus->setTextAlignment(s2d::Alignment::Far, s2d::Alignment::Far);
	_labStatus->setFontSmoothing(2.25f);
}

void MainLayout::setStatus(const std::string& s)
{
	_labStatus->setText(s, 0.25f);
}

void MainLayout::onBtnOpenClick(s2d::Button*)
{
	std::string fileName = selectFile({ "fbx", "json", "etmx" }, SelectFileMode::Open, std::string());

	if (fileExists(fileName))
	{
		_labStatus->setText("Loading...");
		openSelected.invokeInMainRunLoop(fileName);
	}
	else
	{
		_labStatus->setText("No file selected or unable to locate selected file");
	}
}

void MainLayout::onBtnSaveClick(s2d::Button*)
{
	std::string fileName = selectFile(StringList(), SelectFileMode::Save, emptyString);

	if (fileName.empty() || replaceFileExt(fileName, emptyString).empty())
		return;
	
	_labStatus->setText("Saving...");
	saveSelected.invokeInMainRunLoop(fileName);
}

}

