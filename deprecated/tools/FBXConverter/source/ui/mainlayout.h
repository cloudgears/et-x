#include <et-ext/scene2d/scene.h>

namespace et {

class MainLayout : public et::s2d::Layout {
 public:
  ET_DECLARE_POINTER(MainLayout);

 public:
  MainLayout(RenderContext* rc);
  void setStatus(const std::string&);

  ET_DECLARE_EVENT1(openSelected, std::string);
  ET_DECLARE_EVENT1(saveSelected, std::string);
  ET_DECLARE_EVENT1(wireframeChanged, bool);

 private:
  void onBtnOpenClick(s2d::Button*);
  void onBtnSaveClick(s2d::Button*);

 private:
  et::s2d::Font::Pointer _mainFont;
  et::s2d::Label::Pointer _labStatus;
  et::s2d::Button::Pointer _btnWireframe;
};

}  // namespace et