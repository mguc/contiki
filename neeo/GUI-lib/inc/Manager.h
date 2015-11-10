/********************************************************************
 * 
 * CONFIDENTIAL  
 * 
 * ---  
 * 
 * (c) 2014-2015 Antmicro Ltd <antmicro.com>  
 * All Rights Reserved.
 * 
 * NOTICE: All information contained herein is, and remains
 * the property of Antmicro Ltd.
 * The intellectual and technical concepts contained herein
 * are proprietary to Antmicro Ltd and are protected by trade secret
 * or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Antmicro Ltd.
 *
 * ---
 *
 * Created on: 2014-09-01
 *     Author: wstrozynski@antmicro.com 
 *
 */

#ifndef GUIMANAGER_H_
#define GUIMANAGER_H_

#include "tinyxml2.h"
#include "stl.h"

#include <pthread.h>
#include <time.h>

#ifndef __linux__
#include <pkgconf/posix.h>
#endif

#include <unistd.h>
#include <math.h>

#include "GUIQueue.h"
#include "XMLSupport.h"
#include "GUILib.h"
#include "AbstractScreen.h"
#include "Component.h"
#include "CustomView.h"
#include "GUIFont.h"
#include "GridView.h"
#include "GridRow.h"
#include "Image.h"
#include "ListView.h"
#include "Panel.h"
#include "ScrollBar.h"
#include "SwitchButton.h"
#include "ProgressBar.h"
#include "Label.h"
#include "Button.h"
#include "Clock.h"
#include "Popup.h"
#include "Ico.h"
#include "Touch.h"
#include "Event.h"
#include "Painter.h"
#include "ContentManager.h"

using namespace tinyxml2;
namespace guilib {

class Manager {
public:
    static void Initialize(uint32_t xSize, uint32_t ySize, int bppFront, int bppBack, bool rotate90);
    static Manager& GetInstance();

    typedef GUIQueue<guilib::Event> EventQueue;
    typedef GUIQueue<guilib::Popup> PopupQueue;


    enum State {
        Refreshing=0,
        ToTheRight,
        ToTheLeft
    };

    enum ScreenType {
        typeUndefined=0,
        typeGridView,
        typeListView,
        typeCustomView
    };

    enum Mode {
        Rotation0=0,
        Rotation90
    };

    class KeyData {
    public:
        KeyData(): name(""), repeat(100) {}
        string name;
        uint32_t repeat;
    };

    void* operator new(size_t size);
    void operator delete(void* ptr);

    virtual ~Manager();
    Panel* GetTopPanel();
    Panel* GetBottomPanel();

    unsigned int flips;

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

    int32_t GetTopPanelHeight();
    bool GetGlobalTopPanelVisibility();
    int32_t GetTotalHeadersHeight();
    int32_t GetBottomPanelHeight();

    void SetScrollingDuration(uint32_t milliseconds);
    uint32_t GetScrollingDuration(void);

    void SetLayerTransparency(uint32_t id, uint8_t value); // TODO: should probably not be exposed

    void SetActiveScreen(const char* activeScreenId, int8_t direction);
    void SetLoadingImage(const Image& image);
    void SetLoadingImageVisibility(uint8_t visible);
    void SetBackgroundImage(const Image& image);
    void SetBackgroundColour(uint32_t colour);
    void SetLogoImage(const Image& logo);

    void SetTouchAreaModificator(int32_t modificator);
    int32_t GetTouchAreaModificator(void);

    void ProcessTouchPoint(bool touched, uint32_t touchX, uint32_t touchY);

    Popup* GetPopupInstance();
    void AddPopup(Popup* popup);
    int32_t ShowCustomPopup(Popup* popup);
    void ShowPopup(const char* Style);
    void ShowPopup(const char* Style, const char* Message);
    void ShowPopup(const char* Style, uint32_t milliseconds);
    void ShowPopup(const char* Style, const char* Message, uint32_t milliseconds);
    void ClosePopup();

    void SetPopupPointer(Popup* popup);
    void KeepPopupVisible(uint32_t milliseconds);
    bool IsPopupVisible();

    void InitializationFinished();
    int8_t IsInitialozationFinished();
    int8_t IsLoadingIcoVisible();

    void AddScreen(AbstractScreen* screen);
    vector<AbstractScreen*>& GetScreensCollection();
    AbstractScreen* GetLastScreen();
    AbstractScreen* GetActiveScreen();
    AbstractScreen* GetScreen(const char* id);
    AbstractScreen* GetScreen(int id);
    Image& GetBackgroundImage();
    Image& GetLoadingImage();
    State GetState();

    void Anim8(AbstractScreen * startScreen, AbstractScreen * targetScreen, int8_t direction);

    //Containers
    void AddImageContentToContainer(string name, ImageContent * image);
    void BindImageContentToImage(const string& contentName, Image* image);

    void AddCallbackToContainer(string name, Event::CallbackPtr Callback);
    Event::CallbackPtr GetCallbackFromContainer(string name) const;

    void AddFontToFontContainer(string name, GUIFont * font);

    void AddKeyMappingToContainer(uint16_t mask, string name, uint32_t repeat);

    GUIFont const * GetFontFromContainer(const char* name) const;
    GUIFont const * GetDefaultFontFromContainer() const;
    Popup* GetPopupFromContainer(const char* name);
    GUIFont const * TryGetFontPtr(const char* fontName) const;
    //Containers

    //XML
    int32_t BuildFromXML(const char *filename);
    int32_t BuildFromXML(const char *XMLDoc, size_t len);
    Event GetEventWithArguments(const char* eventName) const;
    //XML

    //Physical keys
    void ProcessKeyInput(bool pressed, uint16_t code);
    Key::KeyState PressKey(const char* id);
    Key::KeyState ReleaseKey(const char* id);

    void DrawNextLoadingFrame();
    void Draw();
    void DrawDots(uint32_t numberOfDots, uint32_t activeDot);

    EventQueue& GetEventsQueueInstance();
    void MainLoop();

    void ResetScreens();
    void ClearBuffers();

    void SetExternalContentRequestCallback(ContentManager::ContentCallback requestCallback);
    void SetCancelExternalContentRequestCallback(ContentManager::ContentCallback cancelRequestCallback);

private:
    Manager()
    : width(0), height(0), TopPanel(0), BottomPanel(0), StartHeader(0), TargetHeader(0), ActiveScreen(0), TargetScreen(0),
      AnimationWindowOffset(0), CurrentTimeStamp(0), BeginTimeStamp(0), PointerTimeStamp(0), KeyPressTimeStamp(0), ManagerState(Refreshing),
      LoadingImage(), BackgroundImage(), LoadingProcessCompleted(0), LoadingImageVisible(0), CurrentPopup(0), ScrollingDuration(800),
      TouchEvent(), LogoImage(), painter(), previousTouchState(false), touchToPopup(false), skipDrawing(false), timeoutedPopupMode(false), longPressActive(false),
      keyActive(false), touchAreaModificator(0), dotColor(0), dotActiveColor(0), dotDistance(0), dotRadius(0), dotYPos(0), debugDot(0) {
        flips = 0;
    }
    Manager(uint32_t xSize, uint32_t ySize, int bppFront, int bppBack, bool rotate90); //Regular constructor

    static Manager *instance;

    uint32_t width;
    uint32_t height;
    typedef map<string, Event::CallbackPtr> EventsContainerMap;
    typedef map<string, GUIFont const *> FontContainerMap;
    typedef map<uint32_t, KeyData> KeyMappingMap;
    EventsContainerMap EventsContainer;
    FontContainerMap FontContainer;
    KeyMappingMap KeyMappingContainer;
    vector<AbstractScreen*> Screens;
    vector<Popup*> PopupsContainer;
    Panel* TopPanel;
    Panel* BottomPanel;
    Panel* StartHeader;
    Panel* TargetHeader;
    AbstractScreen *ActiveScreen, *TargetScreen;
    int32_t AnimationWindowOffset; //0 to XSize
    uint64_t CurrentTimeStamp, BeginTimeStamp, PointerTimeStamp, KeyPressTimeStamp;
    State ManagerState;
    Image LoadingImage;
    Image BackgroundImage;
    uint8_t LoadingProcessCompleted;
    uint8_t LoadingImageVisible;
    Popup* CurrentPopup;
    uint64_t ScrollingDuration;
    Touch TouchEvent;
    Image LogoImage;
    Painter painter;
    bool previousTouchState;
    bool touchToPopup;
    bool skipDrawing;
    bool timeoutedPopupMode;
    bool longPressActive, keyActive;
    int32_t touchAreaModificator;
    uint32_t dotColor, dotActiveColor;
    int32_t dotDistance, dotRadius, dotYPos, debugDot;
    KeyData activeKey;
    EventQueue eventsQueue;
    PopupQueue popupsQueue;
    ContentManager contentManager;

    pthread_mutex_t DrawMutex;
    pthread_mutexattr_t DrawMutexAttr;
    //XML private
    AbstractScreen* BuildScreen(XMLElement* ScreenNode);
    void ParseGuiConfiguration(XMLElement* ConfigNode);
    void ParseKeypadMapping(XMLElement* KeypadNode);
    //XML end

    void UpdateAnimationWindowOffset();
    void _ShowPopup();
    void _ClosePopup();
};

} /* namespace guilib */

#endif /* GUIMANAGER_H_ */
