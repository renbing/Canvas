//
//  MovieClip.cpp
//  Canvas
//
//  Created by kins on 11/21/12.
//  Copyright (c) 2012 kins. All rights reserved.
//

#include "MovieClip.h"

void MovieClip::stopAtHead(isRecursive) {
    gotoAndStop(1);
    if( !isRecursive ) return;

    vector<DisplayObject *> &children = frames[currentFrame];
    for( int i=0,max=children.size(); i<max; i++ ) {
        DisplayObject *child = children[i]; 
        if( typeid(*child) == typeid(MovieClip) ) {
            child->stopAtHead(isRecursive);
        }
    }
}

void MovieClip::addChild(DisplayObject *obj) {
    if( !obj ) return;
    if( obj->parent ) {
        obj->parent.removeChild(obj);
    }
    
    frames[currentFrame].push_back(obj);
    obj->parent = this;
}

void MovieClip::addChildAt(DisplayObject *obj, unsigned int frameIndex) {
    if( !obj ) return;
    if( obj->parent ) {
        obj->parent.removeChild(obj);
    }

    vector<DisplayObject *> &children = frames[currentFrame];
    children.insert(children.begin() + frameIndex, obj);
    obj->parent = this;
}

DisplayObject * MovieClip::getChildByName(const string &name) {
    vector<DisplayObject *> &children = frames[currentFrame];
    for( int i=0,max=children.size(); i<max; i++ ) {
        DisplayObject *child = children[i];
        if( typeid(*child) == typeid(MovieClip) && child->name == name ) {
            return child;
        }
    }

    return NULL;
}

DisplayObject * MovieClip::getChildAt(unsigned int index) {
    vector<DisplayObject *> &children = frames[currentFrame];
    if( index < 0 || index >= children.size() ) {
        return NULL;
    }

    return children[index];
}

void MovieClip::removeChildByName(const string &name) {
    vector<DisplayObject *> &children = frames[currentFrame];
    for( int i=0,max=children.size(); i<max; i++ ) {
        DisplayObject *child = children[i];
        if( typeid(*child) == typeid(MovieClip) && child->name == name ) {
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
    if( index < 0 || index >= children.size() ) return;

    children.erase(children.begin() + index);
}

void MovieClip::removeAllChild() {
    vector<DisplayObject *> &children = frames[currentFrame];
    children.clear();
}

void MovieClip::removeFromParent() {
    if( !parent ) return;

    parent.removeChild(this);
}

float MovieClip::getWidth() {
    return 100;
}

float MovieClip::getHeight() {
    return 100;
}

void MovieClip::addEventListener(EventType type, EventCallback *callbak) {
    if( eventBubbleCallBack.find(type) == eventBubbleCallBack.end() ) {
        eventBubbleCallBack[type] = vector<EventCallback *>();
    }
    callbackHash[type].push_back(callback)
}

void MovieClip::removeEventListener(EventType type, EventCallback *callbak) {
    if( eventBubbleCallBack.find(type) == eventBubbleCallBack.end() ) return;

    vector<EventCallback *> &callbacks = eventBubbleCallBack[type];
    if( callback ) {
        for(vector<EventCallback *>::iterator it=callbacks.begin(); it!=callbacks.end(); it++) {
            if( *(*it) == *callack ) {
                delete *it;
                callback.erase(it);
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

DisplayObject * hitTest(float x, float y, bool useAlphaTest);
}

void MovieClip::bubbleEvent(Event *e) {
        triggerEvent(e);

        // 如果事件触发中停止了事件冒泡,则停止
        if (e.isStoped) {
            return;
        }

        if( parent ) {
            parent.bubbleEvent(e);
        }
}

void MovieClip::triggerEvent(const Event *e);
    if( eventBubbleCallBack.find(e->type) != eventBubbleCallBack.end() ) {
        vector<EventCallback *> &callbacks = eventBubbleCallBack[e->type];
        for( int i=0,max=callbacks.size(); i<max; i++ ) {
            callbacks[i](this, e);
        }
    }
    // 主动停止冒泡
    e->isStoped = true;
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

    // 处理ENTER_FRAME事件
    if( eventBubbleCallBack.find(Event.ENTER_FRAME) != eventBubbleCallBack.end()) {
        vector<EventCallback *> &callbacks = eventBubbleCallBack[Event.ENTER_FRAME];
        for( int i=0,max=callbacks.size(); i<max; i++ ) {
            callbacks[i](this, e);
        }
    }
        
        var changeAlpha = false;
        var savedAlpha = global.context2d.globalAlpha;
        if (this.alpha < 1 && this.alpha != global.context2d.globalAlpha) {
            changeAlpha = true;
            global.context2d.globalAlpha = this.alpha;
        }

        matrix.tx += matrix.a * this.x;
        matrix.ty += matrix.d * this.y;

        if( this.scaleX != 1 || this.scaleY != 1 ) {
            matrix.a *= this.scaleX;
            matrix.d *= this.scaleY;
        }

        var handleFlipOffset = false;
        if ( (this.flipOffsetX || this.flipOffsetY) && (this.scaleX != 1 || this.scaleY != 1) 
            && (this.scaleX < 0 || this.scaleY < 0 ) ) {
            
            handleFlipOffset = true;
            this.flipOffsetX = this.flipOffsetX || 0;
            this.flipOffsetY = this.flipOffsetY || 0;
            
            matrix.tx += matrix.a * this.flipOffsetX;
            matrix.ty += matrix.d * this.flipOffsetY;
        }

        if (this.clipRect) {
            var px = matrix.a * this.clipRect.x + matrix.tx;
            var py = matrix.d * this.clipRect.y + matrix.ty;

            if (global.isInBrowser) {
                global.context2d.save();
                global.context2d.beginPath();
                global.context2d.rect(
                    px,
                    py,
                    this.clipRect.w,
                    this.clipRect.h);
                global.isShowRect && global.context2d.stroke();
                global.context2d.closePath();
                global.context2d.clip();
            } else {
                global.context2d.setClipRect(
                    px,
                    py,
                    this.clipRect.w,
                    this.clipRect.h);
            }
        }

        var frames = this.frames[this.currentFrame];
        for (var i = 0, j = frames.length - 1; i <= j; i++) {
            var child = frames[i];
            if( !child ) {
                continue;
            }

            if (child instanceof MovieClip) {
                child.render();
            } else if (child instanceof TextField) {
                child.render(matrix.tx, matrix.ty);
            } else if (child instanceof Texture && child.img) {
                var texture = child;

                var px = matrix.a * texture.dx + matrix.tx;
                var py = matrix.d * texture.dy + matrix.ty;

                // 处理缩放,旋转问题
                var specialTransform = false;
                if (matrix.a != 1 || matrix.d != 1) {
                    specialTransform = true;
                }

                if (texture.rotation && (texture.rotation % 360 != 0)) {
                    specialTransform = true;
                }

                if (specialTransform) {
                    global.context2d.save();
                    global.context2d.translate(px, py);

                    if (matrix.a != 1 || matrix.d != 1) {
                        global.context2d.scale(matrix.a, matrix.d);
                    }
                    if (texture.rotation) {
                        var cx = texture.dw * Math.abs(matrix.a) / 2;
                        var cy = texture.dh * Math.abs(matrix.a) / 2;
                        global.context2d.translate(cx, cy);
                        global.context2d.rotate(texture.rotation / 180 * Math.PI);
                        global.context2d.translate(-cx, -cy);
                    }

                    global.context2d.drawImage(texture.img, texture.sx, texture.sy, texture.sw, texture.sh,
                        0, 0, texture.dw, texture.dh);
                    if (global.isInBrowser && global.isShowRect) {
                        global.context2d.beginPath();
                        global.context2d.rect(0, 0, texture.dw, texture.dh);
                        global.context2d.stroke();
                        global.context2d.closePath();
                    }
                    global.context2d.restore();
                } else {
                    global.context2d.drawImage(texture.img, texture.sx, texture.sy, texture.sw, texture.sh,
                                        px, py, texture.dw, texture.dh);
                    if (global.isInBrowser && global.isShowRect) {
                        global.context2d.beginPath();
                        global.context2d.rect(px, py, texture.dw, texture.dh);
                        global.context2d.stroke();
                        global.context2d.closePath();
                    }
                }

                texture.bounds.x = px;
                texture.bounds.y = py;
                texture.bounds.w = matrix.a * texture.dw;
                texture.bounds.h = matrix.d * texture.dh;

            } else if (child instanceof FillRect) {
                var px = matrix.a * child.x + matrix.tx;
                var py = matrix.d * child.y + matrix.ty;

                var fillStyle = global.context2d.fillStyle;
                var globalAlpha = global.context2d.globalAlpha;

                global.context2d.fillStyle = child.color;
                global.context2d.globalAlpha = child.alpha;

                child.bounds.x = px;
                child.bounds.y = py;
                child.bounds.w = matrix.a * child.w;
                child.bounds.h = matrix.d * child.h;

                global.context2d.fillRect(px, py, child.bounds.w + 1, child.bounds.h + 1);

                global.context2d.fillStyle = fillStyle;
                global.context2d.globalAlpha = globalAlpha;
            }

            if( child.bounds && child.bounds.w < 0 ) {
                child.bounds.x += child.bounds.w;
                child.bounds.w = - child.bounds.w;
            }

            if( child.bounds && child.bounds.h < 0 ) {
                child.bounds.y += child.bounds.h;
                child.bounds.h = -child.bounds.h;
            }
        }

        if (!this.isStoped) {
            this.frameFloatCursor += this.frameSpeed;
            this.currentFrame = Math.floor(this.frameFloatCursor);
            if (this.currentFrame > this.totalFrames) {
                if (this.isPlayOnce) {
                    this.gotoAndStop(1);
                    this.isPlayOnce = false;
                    this.visible = false;
                }
                this.currentFrame = 1;
                this.frameFloatCursor = 1;
            }
        }

        if (this.clipRect) {
            if (global.isInBrowser) {
                global.context2d.restore();
            } else {
                global.context2d.resetClipRect();
            }
        }

        if( handleFlipOffset ) {
            matrix.tx -= matrix.a * this.flipOffsetX;
            matrix.ty -= matrix.d * this.flipOffsetY;
        }

        if( this.scaleX != 1 ) {
            matrix.a /= this.scaleX;
        }
        if( this.scaleY != 1 ) {
            matrix.d /= this.scaleY;
        }

        matrix.tx -= matrix.a * this.x;
        matrix.ty -= matrix.d * this.y;


        changeAlpha && (global.context2d.globalAlpha = savedAlpha);
    };

