//
// Created by michiel on 18-7-2015.
//

#include "Viewer2D.h"
#include <qopenglcontext.h>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLShader>
#include <algorithm>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <string>
#include <memory>


void DeleteNothing(void * ptr)
{

}

void DeleteTexture(void * ptr)
{
    if(ptr != nullptr)
        ((QOpenGLTexture*)ptr)->destroy();
}

Viewer2D::Viewer2D(QWidget *parent)
    : QOpenGLWidget(parent), layers()
{
    std::cout << "create layers" << std::endl;
    this->layers.push_back(std::shared_ptr<Layer>(new GridLayer()));
    this->layers.push_back(std::shared_ptr<Layer>(new SceneLayer()));
}

void Viewer2D::SetOperationRunner(std::shared_ptr<OperationRunner> operationRunner)
{
    this->operationRunner = operationRunner;
}


void Viewer2D::paintGL()
{
    auto del = [](int * p) { std::cout << "Deleting x, value is : " << *p; };
    std::unique_ptr<QOpenGLFunctions, void(*)(void*)> f(QOpenGLContext::currentContext()->functions(), DeleteNothing);

    for(int i = 0; i < this->layers.size(); i++)
    {
        this->layers[i]->PaintGL(this, f);
    }
}

void Viewer2D::initializeGL()
{
    std::unique_ptr<QOpenGLFunctions, void(*)(void*)> f(QOpenGLContext::currentContext()->functions(), DeleteNothing);

    f->glClearColor(0, 0, 0, 1);

    f->glDisable(GL_CULL_FACE);

    for(int i = 0; i < this->layers.size(); i++)
    {
        this->layers[i]->InitializeGL(this, f);
    }
}

void Viewer2D::resizeGL(int w, int h)
{
    std::unique_ptr<QOpenGLFunctions, void(*)(void*)> f(QOpenGLContext::currentContext()->functions(), DeleteNothing);
    f->glViewport(0, 0, h, w);
}

QSize Viewer2D::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize Viewer2D::sizeHint() const
{
    return QSize(400, 400);
}

void Viewer2D::UpdateFromModel(std::shared_ptr<Model> const &model)
{
    for(int i = 0; i < this->layers.size(); i++)
    {
        this->layers[i]->UpdateScene(model);
    }
}

void GridLayer::InitializeGL(QObject* context, std::unique_ptr<QOpenGLFunctions, void(*)(void*)> const &f)
{
    std::unique_ptr<std::string> vertexFile = std::unique_ptr<std::string>(new std::string("GPU\\Grid.vert.glsl"));
    std::unique_ptr<std::string> fragmentFile = std::unique_ptr<std::string>(new std::string("GPU\\Grid.frag.glsl"));

    program = std::unique_ptr<QOpenGLShaderProgram>(new QOpenGLShaderProgram(nullptr));
    program->addShaderFromSourceFile(QOpenGLShader::Vertex, QString::fromStdString(*vertexFile));
    program->addShaderFromSourceFile(QOpenGLShader::Fragment, QString::fromStdString(*fragmentFile));
    program->link();

    program->bind();

    static const GLfloat g_vertex_buffer_data[] = {};

    QColor color(255, 255, 0, 255);

    program->enableAttributeArray("a_position");
    program->setUniformValue("u_color", color);
}

void GridLayer::PaintGL(QObject* context, std::unique_ptr<QOpenGLFunctions, void(*)(void*)> const &f)
{
    UpdateLines();

    program->bind();
    program->setUniformValue("u_view", this->viewMatrix);
    program->setAttributeArray("a_position", this->positions->data(), 2);
    f->glLineWidth(1.0f);
    f->glDrawArrays(GL_LINES, 0, lines*2);
}

void GridLayer::UpdateScene(std::shared_ptr<Model> const &m)
{

}

MinMaxValue GridLayer::GetMinMax()
{
    MinMaxValue result;
    result.Active = false;
    return result;
}

void Viewer2D::UpdateViewMatrix(QMatrix4x4 matrix)
{
    this->_view_matrix = matrix;

    std::for_each(this->layers.begin(), this->layers.end(), [matrix](std::shared_ptr<Layer> l){l->UpdateViewMatrix(matrix);});
}

void GridLayer::UpdateLines()
{
    float grid_spacing = 0.2f;

    QVector2D tl = (this->viewMatrix.inverted() * QVector3D(-1, -1, 0)).toVector2D();
    QVector2D br = (this->viewMatrix.inverted() * QVector3D(1, 1, 0)).toVector2D();

    tl[0] = floorf(tl[0]/grid_spacing);
    tl[1] = floorf(tl[1]/grid_spacing);
    br[0] = ceilf(br[0]/grid_spacing);
    br[1] = ceilf(br[1]/grid_spacing);

    unsigned int verticalLines = (unsigned int)fabsf(br[0]-tl[0]);
    unsigned int horizontalLines = (unsigned int)fabsf(br[1]-tl[1]);

    unsigned int lines = verticalLines+horizontalLines;

    tl[0] = tl[0]*grid_spacing;
    tl[1] = tl[1]*grid_spacing;
    br[0] = br[0]*grid_spacing;
    br[1] = br[1]*grid_spacing;

    std::unique_ptr<std::vector<float> > positions = std::unique_ptr<std::vector<float> >(new std::vector<float>(lines*4));
    for(int i = 0; i < verticalLines; i++)
    {
        (*positions)[i*4+0] = tl[0]+i*grid_spacing;
        (*positions)[i*4+1] = tl[1];
        (*positions)[i*4+2] = tl[0]+i*grid_spacing;
        (*positions)[i*4+3] = br[1];
    }

    for(int i = 0; i < horizontalLines; i++)
    {
        int offset = verticalLines*4;
        (*positions)[offset+i*4+0] = tl[0];
        (*positions)[offset+i*4+1] = tl[1]+i*grid_spacing;
        (*positions)[offset+i*4+2] = br[0];
        (*positions)[offset+i*4+3] = tl[1]+i*grid_spacing;
    }

    this->positions = std::move(positions);
    this->lines = lines;
}

MinMaxValue::MinMaxValue()
{

}

MinMaxValue::MinMaxValue(QVector2D min, QVector2D max)
{
    this->min = min;
    this->max = max;
    this->Active = true;
}

MinMaxValue MinMaxValue::Combine(MinMaxValue first, MinMaxValue second)
{
    using namespace boost::numeric::ublas;
    MinMaxValue result;
    if(!first.Active && !second.Active)
    {
        result.Active = false;
    }
    else if(!first.Active)
    {
        for (int i = 0; i < 2; i++)
        {
            result.min[i] = second.min[i];
            result.max[i] = second.max[i];
        }
    }
    else if(!second.Active)
    {
        for (int i = 0; i < 2; i++)
        {
            result.min[i] = first.min[i];
            result.max[i] = first.max[i];
        }
    }
    else
    {
        for (int i = 0; i < 2; i++)
        {
            result.min[i] = std::min(first.min[i], second.min[i]);
            result.max[i] = std::max(first.max[i], second.max[i]);
        }
    }
    return result;
}

MinMaxValue MinMaxValue::CombineList(std::vector<MinMaxValue> list)
{
    MinMaxValue result = list.back();
    list.pop_back();
    for(MinMaxValue v: list)
    {
        result = Combine(result, v);
    }
    return result;
}

void GLError(std::string name)
{
    std::cout << "------------------------------------------------------" <<std::endl;
    std::cout << "=" << name << std::endl;
    GLenum err = GL_NO_ERROR;
    while((err = glGetError()) != GL_NO_ERROR)
    {
        std::cout << err << ": " << gluErrorString(err) <<std::endl;
    }
}

void SceneLayer::InitializeGL(QObject *context, std::unique_ptr<QOpenGLFunctions, void (*)(void *)> const &f)
{
    std::unique_ptr<std::string> vertexFile = std::unique_ptr<std::string>(new std::string("GPU\\Scene2D.vert"));
    std::unique_ptr<std::string> fragmentFile = std::unique_ptr<std::string>(new std::string("GPU\\Scene2D.frag"));

    program = std::unique_ptr<QOpenGLShaderProgram>(new QOpenGLShaderProgram(nullptr));
    program->addShaderFromSourceFile(QOpenGLShader::Vertex, QString::fromStdString(*vertexFile));
    program->addShaderFromSourceFile(QOpenGLShader::Fragment, QString::fromStdString(*fragmentFile));
    program->link();

    program->bind();

    CreateColormap();

    program->enableAttributeArray("a_position");
    program->enableAttributeArray("a_value");
    program->setUniformValue("colormap", this->textureID);
    program->setUniformValue("vmin", 0.0f);
    program->setUniformValue("vmax", 1.0f);
}

void SceneLayer::PaintGL(QObject *context, std::unique_ptr<QOpenGLFunctions, void (*)(void *)> const &f)
{
    program->bind();
    program->setUniformValue("u_view", this->viewMatrix);
    program->setAttributeArray("a_position", this->positions->data(), 2);
    program->setAttributeArray("a_value", this->values->data(), 2);
    f->glLineWidth(5.0f);
    f->glDrawArrays(GL_LINES, 0, lines*2);
}

void SceneLayer::UpdateScene(std::shared_ptr<Model> const &m)
{
    std::shared_ptr<rapidjson::Document> conf = m->d->GetSceneConf();

    rapidjson::Value& domains = (*conf)["domains"];

    this->positions->clear();
    this->values->clear();
    this->lines = 0;

    for(rapidjson::SizeType i = 0; i < domains.Size(); i++)
    {
        QVector2D tl(domains[i]["topleft"][0].GetDouble(), domains[i]["topleft"][1].GetDouble());
        QVector2D size(domains[i]["size"][0].GetDouble(), domains[i]["size"][1].GetDouble());
        QVector2D br = tl+size;
        float left = tl[0];
        float top = tl[1];

        float right = br[0];
        float bottom = br[1];

        float aTop = domains[i]["edges"]["t"]["a"].GetDouble();
        float aBottom = domains[i]["edges"]["b"]["a"].GetDouble();
        float aLeft = domains[i]["edges"]["l"]["a"].GetDouble();
        float aRight = domains[i]["edges"]["r"]["a"].GetDouble();

        this->positions->push_back(left); this->positions->push_back(top);
        this->values->push_back(aTop);
        this->positions->push_back(right); this->positions->push_back(top);
        this->values->push_back(aTop);

        this->positions->push_back(left); this->positions->push_back(top);
        this->values->push_back(aLeft);
        this->positions->push_back(left); this->positions->push_back(bottom);
        this->values->push_back(aLeft);

        this->positions->push_back(left); this->positions->push_back(bottom);
        this->values->push_back(aBottom);
        this->positions->push_back(right); this->positions->push_back(bottom);
        this->values->push_back(aBottom);

        this->positions->push_back(right); this->positions->push_back(top);
        this->values->push_back(aRight);
        this->positions->push_back(right); this->positions->push_back(bottom);
        this->values->push_back(aRight);

        this->lines += 4;
    }
}

MinMaxValue SceneLayer::GetMinMax()
{
    return MinMaxValue();
}



void SceneLayer::CreateColormap()
{
    std::unique_ptr<std::vector<float>> colormap(new std::vector<float>(2*512*4));
    for(int i = 0; i < 512; i++)
    {
        (*colormap)[i*4+0] = i/512.0f;
        (*colormap)[i*4+1] = 0;
        (*colormap)[i*4+2] = 1-i/512.0f;
        (*colormap)[i*4+3] = 1;

        (*colormap)[512*4+i*4+0] = i/512.0f;
        (*colormap)[512*4+i*4+1] = 0;
        (*colormap)[512*4+i*4+2] = 1-i/512.0f;
        (*colormap)[512*4+i*4+3] = 1;
    }

    // Create one OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);
    GLError("glGenTextures");

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, textureID);
    GLError("glBindTexture");

    // Give the image to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 512, 2, 0, GL_RGBA, GL_FLOAT, colormap->data());
    GLError("glTexImage1D");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLError("glTexParameteri");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GLError("glTexParameteri");

    this->textureID = textureID;
}


void Viewer2D::mousePressEvent(QMouseEvent *event)
{
    QPoint ScreenPos = event->pos();
    QVector2D pos = QVector2D(ScreenPos.x()/((float)this->width())*2-1, -1*(ScreenPos.y()/((float)this->height())*2-1));

    std::shared_ptr<LambdaOperation> op(new LambdaOperation([&](const Reciever &reciever){
        reciever.model->mouseHandler->mousePressEvent(reciever.model, event, pos);
    }));
    this->operationRunner->RunOperation(op);
}

void Viewer2D::mouseReleaseEvent(QMouseEvent *event)
{
    QPoint ScreenPos = event->pos();
    QVector2D pos = QVector2D(ScreenPos.x()/((float)this->width())*2-1, -1*(ScreenPos.y()/((float)this->height())*2-1));

    std::shared_ptr<LambdaOperation> op(new LambdaOperation([&](const Reciever &reciever){
        reciever.model->mouseHandler->mouseReleaseEvent(reciever.model, event, pos);
    }));
    this->operationRunner->RunOperation(op);
}

void Viewer2D::mouseMoveEvent(QMouseEvent *event)
{
    QPoint ScreenPos = event->pos();
    QVector2D pos = QVector2D(ScreenPos.x()/((float)this->width())*2-1, -1*(ScreenPos.y()/((float)this->height())*2-1));

    std::shared_ptr<LambdaOperation> op(new LambdaOperation([&](const Reciever &reciever){
        reciever.model->mouseHandler->mouseMoveEvent(reciever.model, event, pos);
    }));
    this->operationRunner->RunOperation(op);
}

void Viewer2D::wheelEvent(QWheelEvent *event)
{
    QPoint ScreenPos = event->pos();
    QVector2D pos = QVector2D(ScreenPos.x()/((float)this->width())*2-1, -1*(ScreenPos.y()/((float)this->height())*2-1));

    std::shared_ptr<LambdaOperation> op(new LambdaOperation([&](const Reciever &reciever){
        reciever.model->mouseHandler->wheelEvent(reciever.model, event, pos);
    }));
    this->operationRunner->RunOperation(op);
}
