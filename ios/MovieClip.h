//
//  MovieClip.h
//  Canvas
//
//  Created by kins on 11/21/12.
//  Copyright (c) 2012 kins. All rights reserved.
//

#ifndef __Canvas__MovieClip__
#define __Canvas__MovieClip__

#include <JavaScriptCore/JavaScriptCore.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>

using std::vector;
using std::string;
using std::map;

typedef struct
{
    float r,g,b,a;
}Color4f;

typedef struct
{
    float x,y,w,h;
}Rect4f;

typedef enum {
    MOUSE_DOWN = 1,
    MOUSE_MOVE = 2,
    MOUSE_UP = 3,
    ENTER_FRAME = 4
}EventType;

typedef struct
{
    EventType type;
    bool isStoped;
    void *data;
}Event;

class MovieClip;

class EventCallback
{
    public:
        virtual void call(const MovieClip *obj, const Event *e = NULL) = 0;
        virtual ~EventCallback(){}
};

class JSEventCallback : public EventCallback
{
    public:
        JSObjectRef callback;

    public:
        JSEventCallback(JSObjectRef callback) : callback(callback){}

        virtual void call(const MovieClip *obj, const Event *e = NULL) {
        
        }

        bool operator==(const JSEventCallback &target) {
            return callback == target.callback;            
        }
};

class DisplayObject
{
    public:
        MovieClip *parent;
    
        virtual void render() = 0;
        virtual ~DisplayObject(){};
};

class Texture : public DisplayObject
{
    public:
        int textureId;

        float sx;
        float sy;
        float sw;
        float sh;

        float dx;
        float dy;
        float dw;
        float dh;

    public:
        virtual void render();

};

class FillRect : public DisplayObject
{
    public:
        float x;
        float y;
        float w;
        float h;

    private:
        Color4f m_color;
    
    public:
        // RGBA
        string & get_color();
        void set_color(string color);

    public:
        virtual void render();
};

class MovieClip : public DisplayObject
{
    public:
        float x;
        float y;
        string name;
        bool visible;
        float rotation;
        float scaleX;
        float scaleY;
        float alpha;
        float frameSpeed;
        Rect4f clipRect;
    
    private:
        unsigned int totalFrames;
        unsigned int currentFrame;
        //frameSpeed = global.gameFPS / global.renderFPS;
        float frameFloatCursor;
        bool isStoped;
        bool useAlphaTest;
        map<EventType, vector<EventCallback*> > eventBubbleCallBack;
        vector< vector<DisplayObject*> > frames;

    public:
        MovieClip(const string &name="", const unsigned int frameCounts=1);

        virtual void render();

        // 停止播放当前MovieClip,不影响子节点是否播放
        void stop(){
            isStoped = true;
        };

        // 播放当前MovieClip,不影响子节点是否播放
        void play() {
            isStoped = false;
        };

        /* 跳到指定帧,并停止播放
         * @param currentFrame: 指定停止的帧位置 1/2
         */
        void gotoAndStop(unsigned int frame) {
            gotoFrame(currentFrame);
            stop();
        };

        /* 跳到指定帧,并开始播放
         * @param currentFrame: 指定停止的帧位置 1/2
         */
        void gotoAndPlay(unsigned int frame) {
            gotoFrame(currentFrame);
            play();
        }

        /* 停在时间轴头部
         * @param isRecursive: 是否递归到子节点
         */
        void stopAtHead(bool isRecursive=false);

        // 播放下一帧
        void nextFrame() {
            gotoFrame(currentFrame + 1);
        }

        // 播放上一帧
        void prevFrame() {
            gotoFrame(currentFrame - 1);
        }

        /* 添加子节点(放到容器最尾端)
         * @param mc: 子节点对象 MovieClip/Texture/FillRect/TextField
         */
        void addChild(DisplayObject *obj);

        /* 根据层次位置添加子节点
         * @param mc: 子节点对象 MovieClip/Texture/FillRect/TextField
         * @param addIndex: 子节点所在层次
         */
        void addChildAt(DisplayObject *obj, unsigned int frameIndex);


        // 获取当前帧所有子节点
        vector<DisplayObject *> & getChildren() {
            return frames[currentFrame];
        }

        /* 根据名字来获取子节点(只能获取MovieClip)
         * 如果有多个重名的子节点,返回第一个(禁止素材中出现多个重名的子节点)
         * @param name: 子节点名字 child1
         */
        DisplayObject * getChildByName(const string &name);

        /* 根据位置获取子节点
         * @param index: 位置 0/1/2
         */
        DisplayObject * getChildAt(unsigned int index);

        /* 根据名字来删除子节点(只能获取MovieClip)
         * 如果有多个重名的子节点,都会删除(禁止素材中出现多个重名的子节点)
         * @param name: 子节点名字 child1
         */

        void removeChildByName(const string &name);

        /* 删除子节点
         * @param child: 子节点对象
         */
        void removeChild(DisplayObject *obj);

        /* 删除子节点,根据位置
         * @param index: 子节点位置
         */
        void removeChildAt(unsigned int index);

        // 删除所有子节点
        void removeAllChild();

        // 从父节点中删除自己
        void removeFromParent();

        // 获取宽度
        float getWidth();

        // 获取高度
        float getHeight();

        // 增加事件监听
        void addEventListener(EventType type, EventCallback *callbak);

        /* 卸载事件监听
         * @param callback: 如果callback=NULL,会删除所有该类型事件
         */
        void removeEventListener(EventType type, EventCallback *callback = NULL);
        
        // 卸载所有事件
        void removeAllEventListener();

        /* 点碰撞检测
         * 如果开启Alpha测试,会传递给子节点
         */
        DisplayObject * hitTest(float x, float y, bool useAlphaTest = false);

        /* 冒泡事件
         * 从子节点到父节点,递归到舞台
         */
        void bubbleEvent(Event *e);

        /* 触发一个事件
         * 会调用该MovieClip所挂载的所有该事件类型对应的回调
         */
        void triggerEvent(Event *e);

        /* 找到冒泡某个事件对应的第一个响应MovieClip
         * @param eventType: 事件类型 GESTURE_DRAG/GESTURE_SWIPE
         */
        void bubbleFirstResponser(EventType type);

        // 让MovieClip只播放一次,不循环
        void playOnce();

    private:
        void gotoFrame(unsigned int frame);
        void renderFrame();
};

#endif /* defined(__Canvas__MovieClip__) */
