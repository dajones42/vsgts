//	main for vsg train simulator
//
/*
Copyright © 2026 Doug Jones

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <vsg/all.h>
#include <vsgXchange/all.h>
#include <vsgImGui/RenderImGui.h>
#include <vsgImGui/SendEventsToImGui.h>
#include <iostream>
#include <chrono>

#include "mstsace.h"
#include "mstsshape.h"
#include "mstsroute.h"
#include "mstsfile.h"
#include "camerac.h"
#include "tsgui.h"
#include "train.h"
#include "listener.h"
#include "ttosim.h"
#include "timetable.h"

void initSim(vsg::ref_ptr<vsg::Group>& root)
{
	if (trainList.size()==0 && mstsRoute && mstsRoute->activityName.size()==0)
		TSGuiData::instance().loadActivityList();
}

void updateSim(double dt, vsg::ref_ptr<vsg::Group>& root, vsg::ref_ptr<vsg::Viewer>& viewer)
{
	if (trainList.size()==0 && mstsRoute && mstsRoute->activityName.size()>0) {
		auto railCars= vsg::Group::create();
		mstsRoute->activityName+= ".act";
		mstsRoute->loadActivity(railCars.get(),-1);
		cerr<<"activity "<<mstsRoute->activityName<<"\n";
		mstsRoute->activityName.clear();
		cerr<<trainList.size()<<" trains\n";
		auto cr= viewer->compileManager->compile(railCars);
		updateViewer(*viewer,cr);
		root->addChild(railCars);
		ttoSim.init(false);
		for (auto t: trainList)
			listener.addTrain(t);
		listener.setGain(1);
	}
	if (trainList.size() > 0) {
		simTime+= dt;
		updateTrains(dt);
		ttoSim.processEvents(simTime);
	}
}

int main(int argc, char** argv)
{
	auto options= vsg::Options::create();
	auto windowTraits= vsg::WindowTraits::create();
	windowTraits->windowTitle= "vsgts";
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
		if (auto node= object.cast<vsg::Node>())
			scene->addChild(node);
		arguments.remove(i, 1);
		--i;
	}
	timeTable= new TimeTable();
	timeTable->addRow(timeTable->addStation("start"));
	timeTable->setIgnoreOther(true);
	listener.init();
	if (scene->children.empty()) {
		auto object= vsg::read("/home/daj/msts/ROUTES/StL_NA/StL_NA.tdb", options);
		if (auto node= object.cast<vsg::Node>())
			scene->addChild(node);
		auto railCars= vsg::Group::create();
		mstsRoute->activityName= "NA_10Cake2.act";
		mstsRoute->loadActivity(railCars.get(),-1);
		mstsRoute->activityName.clear();
		scene->addChild(railCars);
		ttoSim.init(false);
		for (auto t: trainList)
			listener.addTrain(t);
		listener.setGain(1);
	}
	auto aLight= vsg::AmbientLight::create();
	aLight->color.set(.5,.5,.5);
	aLight->intensity= 1;
	scene->addChild(aLight);
	auto dLight1= vsg::DirectionalLight::create();
	dLight1->color.set(.5,.5,.5);
	dLight1->intensity= 1;
	dLight1->direction.set(0,-1,-1);
	scene->addChild(dLight1);
	auto dLight2= vsg::DirectionalLight::create();
	dLight2->color.set(.5,.5,.5);
	dLight2->intensity= 1;
	dLight2->direction.set(-1,1,.5);
	scene->addChild(dLight2);
//	vsg::write(scene,"test.vsgt");
	initSim(scene);

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
//	std::cout << "selected " << properties.deviceName << std::endl;
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
        viewer->addEventHandler(vsgImGui::SendEventsToImGui::create());
	if (mstsRoute)
		viewer->addEventHandler(CameraController::create(camera,scene));
#if 0
	auto tb= vsg::Trackball::create(camera);
	tb->panButtonMask= vsg::BUTTON_MASK_3;
	tb->zoomButtonMask= vsg::BUTTON_MASK_2;
	tb->supportsThrow= false;
	viewer->addEventHandler(tb);
#endif

//	auto commandGraph=
//	  vsg::createCommandGraphForView(window, camera, scene);
        auto commandGraph= vsg::CommandGraph::create(window);
        auto renderGraph= vsg::RenderGraph::create(window);
        commandGraph->addChild(renderGraph);
        auto view= vsg::View::create(camera);
        view->addChild(scene);
        renderGraph->addChild(view);
        auto renderImGui= vsgImGui::RenderImGui::create(window, TSGui::create());
        renderGraph->addChild(renderImGui);
	viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});
	viewer->compile();
	for (auto& task: viewer->recordAndSubmitTasks) {
		if (task->databasePager) {
			task->databasePager->
			  targetMaxNumPagedLODWithHighResSubgraphs= 25;
		}
	}

	auto prevTime= std::chrono::system_clock::now();
	while (viewer->advanceToNextFrame()) {
		viewer->handleEvents();
		auto now= std::chrono::system_clock::now();
		double dt= std::chrono::duration<double,std::chrono::seconds::period>(now-prevTime).count();
		prevTime= now;
		updateSim(dt,scene,viewer);
		viewer->update();
		viewer->recordAndSubmit();
		viewer->present();
	}
	return 0;
}
