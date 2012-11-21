//
//  MovieClip.cpp
//  Canvas
//
//  Created by kins on 11/21/12.
//  Copyright (c) 2012 kins. All rights reserved.
//

#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>

#include "MovieClip.h"


void Texture::render() {
}

void FillRect::render() {
    
}

MovieClip::MovieClip(const string & name, const unsigned int frameCounts)
    : name(name),totalFrames(frameCounts){
    
    x = 0;
    y = 0;
    visible = true;
    rotation = 0;
    scaleX = 1;
    scaleY = 1;
    alpha = 1.0;
    frameSpeed = 1/60;
    currentFrame = 1;
    frameFloatCursor = 0;
    isStoped = false;
    useAlphaTest = false;
    parent = NULL;
    
    for( int i=0; i<=totalFrames; i++ ) {
        frames.push_back(vector<DisplayObject*>());
    };
}

void MovieClip::stopAtHead(bool isRecursive) {
    gotoAndStop(1);
    if( !isRecursive ) return;

    vector<DisplayObject *> &children = frames[currentFrame];
    for( int i=0,max=children.size(); i<max; i++ ) {
        DisplayObject *child = children[i]; 
        if( typeid(*child) == typeid(MovieClip) ) {
            ((MovieClip *)child)->stopAtHead(isRecursive);
        }
    }
}

void MovieClip::addChild(DisplayObject *obj) {
    if( !obj ) return;
    if( obj->parent ) {
        obj->parent->removeChild(obj);
    }
    
    frames[currentFrame].push_back(obj);
    obj->parent = this;
}

void MovieClip::addChildAt(DisplayObject *obj, unsigned int frameIndex) {
    if( !obj ) return;
    if( obj->parent ) {
        obj->parent->removeChild(obj);
    }

    vector<DisplayObject *> &children = frames[currentFrame];
    children.insert(children.begin() + frameIndex, obj);
    obj->parent = this;
}

DisplayObject * MovieClip::getChildByName(const string &name) {
    vector<DisplayObject *> &children = frames[currentFrame];
    for( int i=0,max=children.size(); i<max; i++ ) {
        DisplayObject *child = children[i];
        if( typeid(*child) == typeid(MovieClip) && ((MovieClip *)child)->name == name ) {
            return child;
        }
    }

    return NULL;
}

DisplayObject * MovieClip::getChildAt(unsigned int index) {
    vector<DisplayObject *> &children = frames[currentFrame];
    if( index >= children.size() ) {
        return NULL;
    }

    return children[index];
}

void MovieClip::removeChildByName(const string &name) {
    vector<DisplayObject *> &children = frames[currentFrame];
    for( int i=0,max=children.size(); i<max; i++ ) {
        DisplayObject *child = children[i];
        if( typeid(*child) == typeid(MovieClip) && ((MovieClip *)child)->name == name ) {
            children.erase(children.begin() + i);
            return;
        }
    }
}

void MovieClip::removeChild(DisplayObject *obj) {
    if( !obj ) return;

    vector<DisplayObject *> &children = frames[currentFrame];
    vector<DisplayObject *>::iterator it = find(children.begin(), children.end(), obj);
    if( it != children.end() ) {
        children.erase(it);
    }
}

void MovieClip::removeChildAt(unsigned int index) {
    vector<DisplayObject *> &children = frames[currentFrame];
    if( index >= children.size() ) return;

    children.erase(children.begin() + index);
}

void MovieClip::removeAllChild() {
    vector<DisplayObject *> &children = frames[currentFrame];
    children.clear();
}

void MovieClip::removeFromParent() {
    if( !parent ) return;

    parent->removeChild(this);
}

float MovieClip::getWidth() {
    return 100;
}

float MovieClip::getHeight() {
    return 100;
}

void MovieClip::addEventListener(EventType type, EventCallback *callback) {
    if( eventBubbleCallBack.find(type) == eventBubbleCallBack.end() ) {
        eventBubbleCallBack[type] = vector<EventCallback *>();
    }
    eventBubbleCallBack[type].push_back(callback);
}

void MovieClip::removeEventListener(EventType type, EventCallback *callback) {
    if( eventBubbleCallBack.find(type) == eventBubbleCallBack.end() ) return;

    vector<EventCallback *> &callbacks = eventBubbleCallBack[type];
    if( callback ) {
        for(vector<EventCallback *>::iterator it=callbacks.begin(); it!=callbacks.end(); it++) {
            if( *(*it) == *callback ) {
                delete *it;
                callbacks.erase(it);
            }
        }
    }else {
        for(vector<EventCallback *>::iterator it=callbacks.begin(); it!=callbacks.end(); it++) {
            delete *it;
        }
        eventBubbleCallBack[type].clear();
    }
}

void MovieClip::removeAllEventListener() {
    map<EventType, vector<EventCallback*> >::iterator it=eventBubbleCallBack.begin();
    for(; it!=eventBubbleCallBack.end(); it++) {
        vector<EventCallback *> &callbacks = it->second;
        for(vector<EventCallback *>::iterator it=callbacks.begin(); it!=callbacks.end(); it++) {
            delete *it;
        }
        callbacks.clear();
    }
}

DisplayObject * MovieClip::hitTest(float x, float y, bool useAlphaTest) {
    return NULL;
}

void MovieClip::bubbleEvent(Event *e) {
    triggerEvent(e);

    // 如果事件触发中停止了事件冒泡,则停止
    if (e->isStoped) {
        return;
    }

    if( parent ) {
        parent->bubbleEvent(e);
    }
}

void MovieClip::triggerEvent(Event *e) {
    if( eventBubbleCallBack.find(e->type) != eventBubbleCallBack.end() ) {
        vector<EventCallback *> &callbacks = eventBubbleCallBack[e->type];
        for( int i=0,max=callbacks.size(); i<max; i++ ) {
            callbacks[i]->call(this, e);
            
            // 主动停止冒泡
            e->isStoped = true;
        }
    }
    
}

void MovieClip::bubbleFirstResponser(EventType type) {
}

void MovieClip::gotoFrame(unsigned int frame) {
    if( frame < 1 ) {
        currentFrame = totalFrames;
    }else if( frame > totalFrames ) {
        currentFrame = 1;
    }else {
        currentFrame = frame;
    }
    frameFloatCursor = currentFrame;
}

void MovieClip::render() {
    if( !visible || scaleX == 0 || scaleY == 0 ) {
        return;
    }
    
    glPushMatrix();
    glTranslatef(x, y, 0);
    
    if( scaleX != 1 && scaleY != 1 ) {
        glScalef(scaleX, scaleY, 0);
    }
    
    if( rotation != 0 ) {
        glRotatef(rotation, 0, 0, 1.0);
    }

    // 处理ENTER_FRAME事件
    if( eventBubbleCallBack.find(ENTER_FRAME) != eventBubbleCallBack.end()) {
        vector<EventCallback *> &callbacks = eventBubbleCallBack[ENTER_FRAME];
        for( int i=0,max=callbacks.size(); i<max; i++ ) {
            callbacks[i]->call(this);
        }
    }
    
    vector<DisplayObject *> &children = frames[currentFrame];
    for( int i=0,max=children.size(); i<max; i++ ) {
        DisplayObject *child = children[i];
        if( !child ) continue;
        
        child->render();
    }
    
    glPopMatrix();
}
