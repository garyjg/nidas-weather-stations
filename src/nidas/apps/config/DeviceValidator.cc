/* -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4; -*- */
/* vim: set shiftwidth=4 softtabstop=4 expandtab: */
/*
 ********************************************************************
 ** NIDAS: NCAR In-situ Data Acquistion Software
 **
 ** 2009, Copyright University Corporation for Atmospheric Research
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** The LICENSE.txt file accompanying this software contains
 ** a copy of the GNU General Public License. If it is not found,
 ** write to the Free Software Foundation, Inc.,
 ** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **
 ********************************************************************
*/
#include "DeviceValidator.h"
#include <iostream>
#include <fstream>


const char * DeviceValidator::_DeviceDefinitionStruct::_InterfaceLabels[] = { "Channel", "Board", "Port", };

DeviceValidator::_DeviceDefinitionStruct DeviceValidator::_Definitions[] = {

{"", "", 0, 0, DeviceValidator::_DeviceDefinition::SERIAL},
{"ANALOG_NCAR", "/dev/ncar_a2d", 0, 2, DeviceValidator::_DeviceDefinition::ANALOG},
{"ANALOG_DMMAT", "/dev/dmmat_a2d0", 0, 2, DeviceValidator::_DeviceDefinition::ANALOG},
{"ACDFO3", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"ADC-GV", "/dev/arinc", 0, 9, DeviceValidator::_DeviceDefinition::SERIAL},
{"AMS", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"Butanol_CN_Counter", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"BCPD", "/dev/ttyS", 10, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"CCN", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"CDP", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"CDP016", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"CDP058", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"CFDC", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"CMIGITS3", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"COMR", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"CORAW", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"CO_2000", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"CO_2005", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"CR2HYGROMETER", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"D_GPS", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"DewPointer", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"Fast2DC", "/dev/usbtwod_64_", 0, 0, DeviceValidator::_DeviceDefinition::SERIAL},
{"GPS-GV", "/dev/arinc", 0, 9, DeviceValidator::_DeviceDefinition::SERIAL},
{"Garmin_GPS", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"GTCIMS", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"HARP", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"HARP_ACTINIC_FLUX", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"HARP_IRRADIANCE", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"HGM232", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"HoneywellPPT", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"HOX1", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"HOX2", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"IRIG", "/dev/irig", 0, 9, DeviceValidator::_DeviceDefinition::SERIAL},
{"IRS-C130", "/dev/arinc", 0, 9, DeviceValidator::_DeviceDefinition::SERIAL},
{"IRS-GV", "/dev/arinc", 0, 9, DeviceValidator::_DeviceDefinition::SERIAL},
{"ISAF", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"ITR", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"LAMS", "/dev/lams", 0, 9, DeviceValidator::_DeviceDefinition::SERIAL},
{"LAMS3", "usock::", 41002, 41002, DeviceValidator::_DeviceDefinition::SERIAL},
{"Mensor_6100", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
// TODO: need to adjust MTP to be inside range of 30100-32766
{"MINIDOAS", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"MTP", "usock::", 30100, 40000, DeviceValidator::_DeviceDefinition::UDP},
// TODO: need to get NOAA instruments inside the range of 30100-32766
{"NOAAPANTHER", "usock::", 20000, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"NOAASP2", "usock::", 20000, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"NOAAUCATS", "usock::", 20000, 38807, DeviceValidator::_DeviceDefinition::UDP},
{"NOAA_CSD_O3", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"NONOYO3", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"Novatel_GPS", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"OphirIII", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"PARCELS", "usock::", 30100, 40000, DeviceValidator::_DeviceDefinition::UDP},
{"Paro_DigiQuartz_1000", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"PAN-CIMS", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"PCIMS", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"PIC_CO2", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"PIC1301_CO2", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"PIC2311_CO2", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"PIC_H2O", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"PILS", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"PTRMS", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"QCLS", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"S100", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"S200", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"S300", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"SIDS", "usock::", 41001, 41001, DeviceValidator::_DeviceDefinition::UDP},
{"SMPS", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"SO2", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"SP2", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"STABPLAT", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"TDLH2O", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"TDL_CVI1", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"TDL_CVI2", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"THREEVCPI", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"THREEVC2D", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"TOGA", "usock::", 30100, 32766, DeviceValidator::_DeviceDefinition::UDP},
{"TwoDP", "/dev/usbtwod_32_", 0, 0, DeviceValidator::_DeviceDefinition::SERIAL},
{"TwoD_House", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"UHSAS", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"UHSAS_CU", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"VCSEL", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},
{"Water_CN_Counter", "/dev/ttyS", 1, 12, DeviceValidator::_DeviceDefinition::SERIAL},

};

DeviceValidator * DeviceValidator::_instance = NULL;

DeviceValidator::DeviceValidator() //: 
       //_devMap(_Definitions, _Definitions + sizeof(_Definitions) / sizeof(_DeviceDefinition)) 
{

  int n =  sizeof(_Definitions) / sizeof(_DeviceDefinition);
  for (int i = 0; i < n;  i++) {
    //_devMap.insert ( _devMap.rbegin(), std::pair<std::string, _DeviceDefinition>(_Definitions[i].sensorName, _Definitions[i]));
    _devMap.insert ( std::pair<std::string, _DeviceDefinition>(_Definitions[i].sensorName, _Definitions[i]));
  }
  for (std::map<std::string, _DeviceDefinition>::iterator it = _devMap.begin(); it != _devMap.end(); it++) {

      std::cerr << it->first << std::endl;
//    cerr << it->first << " " << (_DeviceDefinition)it->second.devicePrefix << " " << (_DeviceDefinition)it->second.min << " " << (_DeviceDefinition)it->second.max << endl;

  }

}
