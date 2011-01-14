#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osg/io_utils>
#include "Clipper.h"
#include <osgUtil/SmoothingVisitor>
#include "PLYWriterNodeVisitor.h"
#include "TexturedSource.h"
#include "TexturingQuery.h"
#include "Extents.h"
#include "auv_stereo_geometry.hpp"
#include "calcTexCoord.h"
#include "PLYWriterNodeVisitor.h"
using namespace libsnapper;
using namespace std;

int main( int argc, char **argv )
{
    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc,argv);

    // set up the usage document, in case we need to print out how to use this program.
    arguments.getApplicationUsage()->setDescription(arguments.getApplicationName() +" is the example which demonstrates Depth Peeling");
    arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName()+" filename");
    Stereo_Calib *calib=NULL;
    string base_dir=argv[1];
    string stereo_calib_file_name = "stereo.calib";
    stereo_calib_file_name= base_dir+string("/")+stereo_calib_file_name;

    try {
        calib = new Stereo_Calib( stereo_calib_file_name );
    }
    catch( string error ) {
        cerr << "ERROR - " << error << endl;
        exit( 1 );
    }


    string outfilename;
    if(!arguments.read("--outfile",outfilename)){
        fprintf(stderr,"Need outfile name\n");
        return -1;
    }
    std::string mf=argv[2];
   /* std::string sha2hash;
    int res=checkCached(mf,outfilename,sha2hash);
    if(res == -1)
        return -1;
    else if(res == 1)
        return 0;//Hash is valid
    cout <<"Computing hash\n";*/
    //Differing hash or no hash
    int npos=mf.find("/");
    std::string bbox_file=std::string(mf.substr(0,npos)+"/bbox-"+mf.substr(npos+1,mf.size()-9-npos-1)+".ply.txt");
    printf("SS %s\n",bbox_file.c_str());
    TexturedSource *sourceModel=new TexturedSource(vpb::Source::MODEL,mf,bbox_file);
    osgDB::Registry::instance()->setBuildKdTreesHint(osgDB::ReaderWriter::Options::BUILD_KDTREES);
    osg::Node* model = osgDB::readNodeFile(sourceModel->getFileName().c_str());
    if (model)
    {
        vpb::SourceData* data = new vpb::SourceData(sourceModel);
        data->_model = model;
        data->_extents.expandBy(model->getBound());
        sourceModel->setSourceData(data);
        osg::Geode *geode= dynamic_cast<osg::Geode*>(model);
        if(geode && geode->getNumDrawables()){
            //addDups(geode);
            osg::Drawable *drawable = geode->getDrawable(0);
            sourceModel->_kdTree = dynamic_cast<osg::KdTree*>(drawable->getShape());
        }else{
            std::cerr << "No drawbables \n";
        }

        TexPyrAtlas atlasGen("null",false);
        TexturingQuery *tq=new TexturingQuery(sourceModel,calib->left_calib,atlasGen,true);
        vpb::MyDestinationTile *tile=new vpb::MyDestinationTile("");

        tq->_tile=tile;
        bool projectSucess=tq->projectModel(dynamic_cast<osg::Geode*>(model));
        if(projectSucess){
            //  writeCached(outfilename,sha2hash,tile->texCoordIDIndexPerModel.begin()->second,tile->texCoordsPerModel.begin()->second);
            std::ofstream f(outfilename.c_str());
            osg::Geometry *geom = dynamic_cast< osg::Geometry*>( geode->getDrawable(0));
            geom->setTexCoordArray(0,tile->texCoordsPerModel.begin()->second);
            PLYWriterNodeVisitor nv(f,tile->texCoordIDIndexPerModel.begin()->second);
            model->accept(nv);
        }else
            cerr << "Failed to project\n";
        delete tq;
    }else
        cerr << "Failed to open "<<sourceModel->getFileName() <<endl;

}