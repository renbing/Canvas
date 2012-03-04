var context = canvas.getContext("2d");

function Sprite(img, offsetX, offsetY)
{
	this.img = img;
	this.offsetX = offsetX;
	this.offsetY = offsetY;
	
	this.draw = function(){
		//context.save();
		//context.translate(100,100);
		//context.rotate(30);
		//context.scale(0.5,0.5);
		//context.drawImage(this.img, this.offsetX, this.offsetY, this.img.width, this.img.height,
		//				  this.img.width/4, this.img.height/4, this.img.width*3/4, this.img.height*3/4);
		context.drawImage(this.img, this.offsetX, this.offsetY);
		//context.restore();
	}
	
	this.intersect = function(x, y){
		var minX = this.offsetX - this.img.width * this.img.anchorPointX;
		var maxX = this.offsetX + (1-this.img.anchorPointX) * this.img.width;
		
		var minY = this.offsetY - this.img.height * this.img.anchorPointY;
		var maxY = this.offsetY + (1-this.img.anchorPointY) * this.img.height;
		
		return ( minX <= x && x <= maxX && minY <= y && y <= maxY);
	}
}

var sprites = [];

var background = new Image();
background.src="assets/HelloWorld.png"; 
background.onload = function(){};
sprites.push(new Sprite(background, 0, 0));

var acorn = new Image();
acorn.src="assets/acorn.png";
acorn.anchorPointX = 0.5;
acorn.anchorPointY = 0.5;
acorn.onload = function(){};

var mainLoop = function(){
	for( var i in sprites )
	{
		var sprite = sprites[i];
		sprite.draw();
	}
};

setMainLoop(mainLoop, Math.floor(1000.0/60));
canvas.addEventListener("mousedown", mouseDownEvent, false);
canvas.addEventListener("mousemove", mouseMoveEvent, false);
canvas.addEventListener("mouseup", mouseUpEvent, false);


var moving = false;
var movingSpriteIndex = -1;

function mouseDownEvent(e)
{
	for( var i = sprites.length-1; i >= 0; i-- )
	{
		sprite = sprites[i];
		
		if( sprite.img == background )
		{
			continue;
		}
		
		if( sprite.intersect(e.pageX, e.pageY) )
		{
			movingSpriteIndex = i;
			break;
		}
	}
}

function mouseMoveEvent(e)
{
	if( movingSpriteIndex >= 0 )
	{
		if( !moving )
		{
			sprites.push(sprites[movingSpriteIndex]);
			sprites.splice(movingSpriteIndex, 1);
			movingSpriteIndex = sprites.length - 1;
			moving = true;
		}
		
		var movingSprite = sprites[movingSpriteIndex];
		movingSprite.offsetX = e.pageX;
		movingSprite.offsetY = e.pageY;
	}
}

function mouseUpEvent(e)
{
	if( moving && movingSpriteIndex >= 0 )
	{
		var movingSprite = sprites[movingSpriteIndex];
		movingSprite.offsetX = e.pageX;
		movingSprite.offsetY = e.pageY;
	}
	else
	{
		sprites.push(new Sprite(acorn, e.pageX, e.pageY));
	}
	
	moving = false;
	movingSpriteIndex = -1;
}

var callCount = 0;
var timer;
function timeoutCallback()
{
	callCount++;
	if( callCount > 10 )
	{
		clearInterval(timer);
	}
	alert("call timeoutCallback");
}

setTimeout(timeoutCallback, 1000);
timer = setInterval(timeoutCallback, 100);
