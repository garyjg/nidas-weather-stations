/*
 ********************************************************************
    Copyright by the National Center for Atmospheric Research

    $LastChangedDate: $

    $LastChangedRevision: $

    $LastChangedBy: $

    $HeadURL: $

 ********************************************************************

*/
#include <AnalogSensorFilter.h>
#include <DSMAnalogSensor.h>
#include <Sample.h>
#include <SamplePool.h>
#include <SampleClient.h>
#include <iostream>

using namespace dsm;
using namespace std;


AnalogSensorFilter::AnalogSensorFilter():  rates(0)
{
}
    
AnalogSensorFilter::~AnalogSensorFilter()
{
    delete [] rates;
}

void AnalogSensorFilter::setAnalogSensor(DSMAnalogSensor* sensor)
{
    const std::vector<int>& channels = sensor->getChannels();

cerr << "in AnalogSensorFilter::setAnalogSensor" << endl;
    rates = new int[channels.size()];

    for (int i = 0; i < (int)channels.size(); i++)
	{
        rates[i] = sensor->getSamplingRate(channels[i]);
	}
}

bool AnalogSensorFilter::receive(const dsm::Sample* samp)
	throw(atdUtil::IOException,dsm::SampleParseException)
{
    ShortIntSample* outs =
    	SamplePool<ShortIntSample>::getInstance()->getSample(
		samp->getDataLength());
cerr << "outs = " << hex << outs << dec << endl;
cerr << "datalength = " << samp->getDataLength() << endl;
cerr << "timetag = " << samp->getTimeTag() << endl;
    for (unsigned int i = 0; i < samp->getDataLength(); i++)
	outs->getDataPtr()[i] = ((const short *)samp->getConstVoidDataPtr())[i];
    outs->setTimeTag(samp->getTimeTag());
    outs->setId(samp->getId());
    outs->setDataLength(samp->getDataLength());

    distribute(outs);
    return true;
}
