#include <vsg/all.h>
#include <vsgXchange/all.h>
#include <iostream>
#include "mstsace.h"
#include "mstsshape.h"
#include "mstsroute.h"

vsg::ref_ptr<vsg::Node> createTextureQuad(vsg::ref_ptr<vsg::Data> sourceData,
  vsg::ref_ptr<vsg::Options> options)
{
	auto builder= vsg::Builder::create();
	builder->options= options;
	vsg::StateInfo state;
	state.image= sourceData;
	state.lighting= false;
	vsg::GeometryInfo geom;
	geom.dy.set(0.0f, 0.0f, 1.0f);
	geom.dz.set(0.0f, -1.0f, 0.0f);
	return builder->createQuad(geom, state);
}

int main(int argc, char** argv)
{
	auto options= vsg::Options::create();
	auto windowTraits= vsg::WindowTraits::create();
	windowTraits->windowTitle= "tsviewer";
	vsg::CommandLine arguments(&argc, argv);
	windowTraits->debugLayer= arguments.read({"--debug","-d"});
	windowTraits->apiDumpLayer= arguments.read({"--api","-a"});
	if (arguments.read({"--fullscreen", "--fs"}))
	windowTraits->fullscreen= true;
	if (arguments.read({"--window", "-w"}, windowTraits->width,
	  windowTraits->height)) {
		windowTraits->fullscreen= false;
	}
	arguments.read("--screen", windowTraits->screenNum);
	arguments.read("--display", windowTraits->display);
	if (arguments.errors())
		return arguments.writeErrorMessages(std::cerr);
	options->add(vsgXchange::all::create());
	options->add(MstsAceReaderWriter::create());
	options->add(MstsShapeReaderWriter::create());
	options->add(MstsRouteReader::create());
	options->add(MstsTerrainReader::create());
//	vsg::Logger::instance()->level= vsg::Logger::LOGGER_DEBUG;;

	auto scene= vsg::Group::create();
	for (int i=1; i<argc; ++i) {
		vsg::Path filename= arguments[i];
		auto object= vsg::read(filename, options);
		if (auto node= object.cast<vsg::Node>()) {
			scene->addChild(node);
		} else if (auto data= object.cast<vsg::Data>()) {
			if (auto textureGeometry=
			  createTextureQuad(data, options)) {
				scene->addChild(textureGeometry);
			}
		}
		arguments.remove(i, 1);
		--i;
	}
	if (scene->children.empty()) {
		std::cout<<"No scene loaded."<<std::endl;
		return 1;
	}
	auto aLight= vsg::AmbientLight::create();
	aLight->color.set(.8,.8,.8);
	aLight->intensity= 1;
	scene->addChild(aLight);
	auto dLight1= vsg::DirectionalLight::create();
	dLight1->color.set(.8,.8,.8);
	dLight1->intensity= 1;
	dLight1->direction.set(0,-1,-1);
	scene->addChild(dLight1);
	auto dLight2= vsg::DirectionalLight::create();
	dLight2->color.set(.8,.8,.8);
	dLight2->intensity= 1;
	dLight2->direction.set(-1,1,.5);
	scene->addChild(dLight2);
//	vsg::write(scene,"test.vsgt");

	auto viewer= vsg::Viewer::create();
	vsg::ref_ptr<vsg::Window> window(vsg::Window::create(windowTraits));
	if (!window) {
		std::cout<<"Could not create window."<<std::endl;
		return 1;
	}
	auto instance = window->getOrCreateInstance();
	auto surface = window->getOrCreateSurface();
	auto physicalDevice = instance->getPhysicalDevice(windowTraits->queueFlags, surface,
	  {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU});
	auto properties = physicalDevice->getProperties();
	std::cout << "selected " << properties.deviceName << std::endl;
	window->setPhysicalDevice(physicalDevice);

	viewer->addWindow(window);

	vsg::ComputeBounds computeBounds;
	scene->accept(computeBounds);
	vsg::dvec3 center=
	  (computeBounds.bounds.min+computeBounds.bounds.max)*0.5;
	auto size= computeBounds.bounds.max-computeBounds.bounds.min;
	double radius= vsg::length(size)*0.6;
	double nearFarRatio= 0.0001;
	auto lookAt= vsg::LookAt::create(center+vsg::dvec3(0.0,
	  -radius*3.5, 0.0), center, vsg::dvec3(0.0, 0.0, 1.0));
	if (mstsRoute)
		lookAt= vsg::LookAt::create(center+vsg::dvec3(0,0,radius*3.5),
		  center,vsg::dvec3(0,1,0));
	vsg::ref_ptr<vsg::ProjectionMatrix> perspective;
	perspective= vsg::Perspective::create(30.0,
	  static_cast<double>(window->extent2D().width) /
	  static_cast<double>(window->extent2D().height),
	  nearFarRatio*radius, radius * 4.5);
	auto camera= vsg::Camera::create(perspective, lookAt,
	  vsg::ViewportState::create(window->extent2D()));

	viewer->addEventHandler(vsg::CloseHandler::create(viewer));
	viewer->addEventHandler(vsg::WindowResizeHandler::create());
	auto tb= vsg::Trackball::create(camera);
	tb->panButtonMask= vsg::BUTTON_MASK_3;
	tb->zoomButtonMask= vsg::BUTTON_MASK_2;
	tb->supportsThrow= false;
	viewer->addEventHandler(tb);

	auto commandGraph=
	  vsg::createCommandGraphForView(window, camera, scene);
	viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});
	viewer->compile();
	for (auto& task: viewer->recordAndSubmitTasks) {
		if (task->databasePager) {
			task->databasePager->
			  targetMaxNumPagedLODWithHighResSubgraphs= 25;
		}
	}

	while (viewer->advanceToNextFrame()) {
		viewer->handleEvents();
		viewer->update();
		viewer->recordAndSubmit();
		viewer->present();
	}
	return 0;
}
