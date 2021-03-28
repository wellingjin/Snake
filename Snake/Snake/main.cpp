#include "GLShaderManager.h"
#include "GLTools.h"
#include <glut/glut.h>
#include <string.h>
#include<stdio.h>
#include <stdlib.h>


GLShaderManager shaderManager;


/// 一个方块的结构体
struct SnakeObject {
    GLBatch *triangleBatch; //图元类
    GLfloat *points; //顶点
    GLfloat *color;  //颜色
};
//节点，一条蛇由多个节点组成
struct SnakeNode {
    SnakeObject snake; //当前节点的内容
    SnakeNode * next; //下一个节点的指针
};
//点
struct Point {
    GLfloat x;
    GLfloat y;
};
//方向枚举
typedef enum : int {
    Left,
    Right,
    Up,
    Down
} Direction;

SnakeNode *SnakeHead; //单向链表，用来表示蛇
int objectCount = 0;  //链表长度
GLfloat ScreenWidth = 600; //窗口宽度
GLfloat ScreenHeight = 600; //窗口高度
GLfloat blockSize = 0.02; //方块的大小，以原始尺寸为标准，也就是-1 1
Direction currentDirection = Right; //当前方向
SnakeObject foodObject; //食物
GLfloat speed = 100;  //速度
//方块模版顶点
GLfloat block[] = {
    -1,-1.0f,0.0f,
    -1.0f,1,0.0f,
    1,1.0f,0.0f,
    1,-1.0f,0.0f,
};

//顶点复制
void copyPoint(GLfloat *v,GLfloat *vCopy,int length) {
    for (int i = 0; i<length; i++) {
        v[i] = vCopy[i];
    }
}
//对顶点进行缩放
void scaleBlock(GLfloat *v, GLfloat scale, int length) {
    for (int i = 0; i < length; i++) {
        v[i] *= scale;
    }
}

//移动顶点到指定的坐标，原始坐标原点在中心位置，为了方便理解，业务坐标点改到左上角，point是以左上角为原点的坐标点
void moveTo(GLfloat *v, Point point) {
    v[0] = point.x*2/ScreenWidth - 1;
    v[1] = -point.y*2/ScreenHeight + 1 - 2*blockSize;
    v[3] = point.x*2/ScreenWidth - 1;
    v[4] = -point.y*2/ScreenHeight + 1;
    v[6] = point.x*2/ScreenWidth - 1 + 2*blockSize;
    v[7] = -point.y*2/ScreenHeight + 1;
    v[9] = point.x*2/ScreenWidth - 1 + 2*blockSize;
    v[10] = -point.y*2/ScreenHeight + 1 - 2*blockSize;
}
//根据顶点坐标获取对应的坐标（左上角）
Point getPoint(GLfloat *v) {
    GLfloat x = 0,y = 0;
    x = (v[0] + 1)*ScreenWidth / 2;
    y = (1 - v[7])*ScreenHeight/2;
    return Point{x,y};
}

//两个点是否碰到
bool isTouch(GLfloat *v, GLfloat *t) {
    Point vxy = getPoint(v);
    Point txy = getPoint(t);
    GLfloat size = blockSize*ScreenWidth*2;
    bool hTouch = vxy.x > txy.x && vxy.x < txy.x + size || vxy.x + size > txy.x && vxy.x+ size < txy.x + size;
    bool vTouch = vxy.y > txy.y && vxy.y < txy.y + size || vxy.y + size > txy.y && vxy.y+ size < txy.y + size;
    return hTouch && vTouch;
}

//沿着某个方向移动step个位置
void moveStep(GLfloat *v, Direction direction, GLfloat step) {
    Point xy = getPoint(v);
    switch (direction) {
        case Left:{
            GLfloat x = xy.x-step;
            if (x < 0) {
                x = 0;
            }
            moveTo(v, Point{x,xy.y});
        }
            break;
        case Right:{
            GLfloat x = xy.x+step;
            if (x + blockSize*ScreenWidth > ScreenWidth) {
                x = ScreenWidth - blockSize*ScreenWidth;
            }
            moveTo(v, Point{x,xy.y});
        }
            break;
        case Up: {
            GLfloat y = xy.y-step;
            if (y <0) {
                y = 0;
            }
            moveTo(v, Point{xy.x,y});
        }
            break;
        case Down:{
            GLfloat y = xy.y+step;
            if (y + blockSize*ScreenHeight > ScreenHeight) {
                y = ScreenHeight - blockSize*ScreenHeight;
            }
            moveTo(v, Point{xy.x,y});
        }
            break;
    }
}
//检测是否碰到边界了
bool isLimitStep(GLfloat *v,Direction direction, GLfloat step) {
    Point xy = getPoint(v);
    switch (direction) {
        case Left:{
            GLfloat x = xy.x-step;
            if (x < 0) {
                return true;
            }
        }
            break;
        case Right:{
            GLfloat x = xy.x+step;
            if (x + blockSize*ScreenWidth > ScreenWidth) {
                return true;
            }
        }
            break;
        case Up: {
            GLfloat y = xy.y-step;
            if (y <0) {
                return true;
            }
        }
            break;
        case Down:{
            GLfloat y = xy.y+step;
            if (y + blockSize*ScreenHeight > ScreenHeight) {
                return true;
            }
        }
            break;
    }
    return false;
}
//根据模版创建一个顶点
void createBlock(GLfloat *v, GLfloat scale) {
    for (int i = 0; i<4*3; i++) {
        v[i] = block[i];
    }
    scaleBlock(v, scale, 4*3);
}
//更新食物的坐标
void updateFood() {
    GLfloat x = rand() % (int)ScreenWidth;
    GLfloat y = rand() % (int)ScreenHeight;
    foodObject.points = (GLfloat*)malloc(sizeof(GLfloat) * 12);
    foodObject.color = (GLfloat*)malloc(sizeof(GLfloat) * 4);
    foodObject.color[3] = foodObject.color[0] = 1;
    foodObject.color[2] = foodObject.color[1] = 0;
    createBlock(foodObject.points, blockSize);
    moveTo(foodObject.points, {x,y});
    foodObject.triangleBatch = new GLBatch();
    foodObject.triangleBatch->Begin(GL_TRIANGLE_FAN,4);
    foodObject.triangleBatch->CopyVertexData3f(foodObject.points);
    foodObject.triangleBatch->End();
}
//窗口大小改变时接受新的宽度和高度，其中0,0代表窗口中视口的左下角坐标，w，h代表像素
void ChangeSize(int w,int h){
    glutReshapeWindow(ScreenWidth,ScreenHeight);
//    glViewport(0,0, w, h);
    
}

//新建一个蛇块，并添加到链表后面
void newASnake(Point point, GLfloat *color) {
    SnakeObject snake = SnakeObject();
    snake.points = (GLfloat*)malloc(sizeof(GLfloat) * 12);
    snake.color = (GLfloat*)malloc(sizeof(GLfloat) * 4);
    for (int i=0; i< 4; i++) {
        snake.color[i] = color[i];
    }
    createBlock(snake.points, blockSize);
    moveTo(snake.points, point);
    snake.triangleBatch = new GLBatch();
    snake.triangleBatch->Begin(GL_TRIANGLE_FAN,4);
    snake.triangleBatch->CopyVertexData3f(snake.points);
    snake.triangleBatch->End();
    SnakeNode *newNode = (SnakeNode *)malloc(sizeof(SnakeNode));
    if (objectCount == 0) {
        SnakeHead = newNode;
    }else {
        SnakeNode *lastNode = SnakeHead;
        while (lastNode->next) {
            lastNode = lastNode->next;
        }
        lastNode->next = newNode;
    }
    newNode->snake = snake;
    newNode->next = NULL;
    
    objectCount++;
}

//绘制一个方块
void drawObject(SnakeObject obj) {
    shaderManager.UseStockShader(GLT_SHADER_IDENTITY,obj.color);
    obj.triangleBatch->Draw();
}

//打印文字
void glutPrint(float x, float y, char *text)
{
    glRasterPos2f(x,y);
    glColor3f(0, 0, 0);
    for (int i=0; text[i]; i++){
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, text[i]);
    }
}
//绘制所有的内容
void drawAll() {
    //清除对应的缓存，防止旧的渲染依然展示在屏幕上
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    //分数文字展示
    char str[20];
    sprintf(str, "score:%d", objectCount);
    glutPrint(-0.9,0.95,str);
    //蛇的展示
    SnakeNode *node = SnakeHead;
    while (node) {
        SnakeObject snake = node->snake;
        drawObject(snake);
        node = node->next;
    }
    //食物
    drawObject(foodObject);
    //将在后台缓冲区进行渲染，然后在结束时交换到前台，也就是开始进行渲染
    glutSwapBuffers();
    
}

//为程序作一次性的设置
void SetupRC() {
    //设置背影颜色
    glClearColor(1.0f,1.0f,1.0f,1.0f);
    //初始化着色管理器
    shaderManager.InitializeStockShaders();
    //初始化一条蛇
    GLfloat x = blockSize*ScreenWidth*10;
    for (int i=0; i<10; i++) {
        GLfloat color[] = {5/10.0f,5/10.0f,9/10.0f,1.0f};
        newASnake(Point{x,300},color);
        x-=blockSize*ScreenWidth;
    }
    //初始化一个食物
    updateFood();
}



//开始渲染，每次刷新都会调用
void RenderScene(void) {
    drawAll();
}
//是否吃到食物
bool eatFood() {
    SnakeObject snake = SnakeHead->snake;
    if (isTouch(snake.points, foodObject.points)) {
        updateFood();
        speed-=10;
        if (speed < 10) {
            speed = 10;
        }
        return true;
    }
    return false;
}
//蛇移动
void move() {
    GLfloat step = blockSize*ScreenWidth;
    if (objectCount == 1) {
        SnakeObject snake = SnakeHead->snake;
        moveStep(snake.points, currentDirection, step);
        snake.triangleBatch->CopyVertexData3f(snake.points);
        glutPostRedisplay();
        return;
    }
    SnakeNode *lastNode = SnakeHead;
    SnakeNode *lastTwoNode = NULL;
    while (lastNode->next) {
        if (lastNode->next->next == NULL) {
            lastTwoNode = lastNode;
            lastNode = lastNode->next;
            break;
        }
        lastNode = lastNode->next;
    }
    SnakeObject snake = lastNode->snake;
    //保存最后一个节点的位置，如果吃到食物则要新增一个顶点在这个位置
    Point lastPoint = getPoint(snake.points);
    
    copyPoint(snake.points, SnakeHead->snake.points, 12);
//碰到边界了，则不动
    if (isLimitStep(SnakeHead->snake.points,currentDirection,step)) {
        return;
    }
    
    moveStep(snake.points, currentDirection, step);
    snake.triangleBatch->CopyVertexData3f(snake.points);
    lastNode->next = SnakeHead;
    lastTwoNode->next = NULL;
    SnakeHead = lastNode;
    if (eatFood()) {//吃到食物新增一个顶点
        GLfloat color[] = {5/10.0f,5/10.0f,9/10.0f,1.0f};
        newASnake(lastPoint, color);
    }
    //重绘
    glutPostRedisplay();
}
//键盘监听
void OnKeyTap(unsigned char key, int x, int y) {
    Direction nextDirection = currentDirection;
    if (key == 'w') {
        nextDirection = Up;
    }else if (key == 'd') {
        nextDirection = Right;
    }else if (key == 's') {
        nextDirection = Down;
    } else if (key == 'a') {
        nextDirection = Left;
    }
    //方向相反无法操作
    if (nextDirection == Left && currentDirection == Right || nextDirection == Right && currentDirection ==Left
        || nextDirection == Up && currentDirection == Down || nextDirection == Down && currentDirection == Up) {
        return;
    }
    currentDirection = nextDirection;
}
//定时方法
void timerFunc(int nTimerID) {
    move();
    glutTimerFunc(speed,timerFunc,1);
}

int main(int argc,char* argv[]){
    
    //设置当前工作目录，针对MAC OS X
    gltSetWorkingDirectory(argv[0]);
    //初始化GLUT库
    glutInit(&argc, argv);
    /*初始化双缓冲窗口，其中标志GLUT_DOUBLE、GLUT_RGBA、GLUT_DEPTH、GLUT_STENCIL分别指
     
     双缓冲窗口、RGBA颜色模式、深度测试、模板缓冲区*/
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL);
    //GLUT窗口大小，标题窗口
    glutInitWindowSize(800,600);
    glutCreateWindow("贪吃蛇");
    //注册回调函数
    glutReshapeFunc(ChangeSize);//窗口改变大小
    
    glutDisplayFunc(RenderScene);//绘制刷新
    glutKeyboardFunc(OnKeyTap);//键盘监听
    glutTimerFunc(speed,timerFunc,1);//定时回调，第一个参数是定时间隔，单位毫秒，第二个回调方法指针，第三个是回调ID，会透传给回调方法的参数
    
    //驱动程序的初始化中没有出现任何问题。
    GLenum err = glewInit();
    if(GLEW_OK != err) {
        fprintf(stderr,"glew error:%s\n",glewGetErrorString(err));
        return 1;
    }
    //调用SetupRC
    SetupRC();
    glutMainLoop(); //启动runloop
    
    return 0;
    
}

