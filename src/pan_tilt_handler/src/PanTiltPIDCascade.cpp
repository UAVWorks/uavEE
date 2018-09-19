////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2018 University of Illinois Board of Trustees
//
// This file is part of uavEE.
//
// uavAP is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// uavAP is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////
	/*headingRatePID_ = controlEnv_.addPID(antennaTarget->heading,headingPID_,defaultParams);
	 controlEnv_.addOutput(headingRatePID_,antennaOutput->panOut);
	 //controlEnv_.addOutput();

	 auto pitchRateInput = controlEnv_.addInput(&antennaSD->angularRate[1]);
	 auto pitchTarget = controlEnv_.addInput(&antennaTarget->tilt);
	 tiltPID_ = controlEnv.addPID(pitchTarget,pitchRateInput,defaultParams);
	 controlEnv_.addOutput(tiltPID_,antennaOutput->tiltOut);*/
}

bool
PanTiltPIDCascade::configure(const boost::property_tree::ptree& config)
{
	APLOG_WARN << "recieved call to configure";
	PropertyMapper pm(config);
	boost::property_tree::ptree pidConfig;
	pm.add("pids", pidConfig, false);
	Control::PID::Parameters params;
	for (auto it : pidConfig)
	{
		APLOG_WARN << "Added PID";
		auto pid = PanTiltPIDBimapRight.find(it.first);
		if (pid == PanTiltPIDBimapRight.end())
		{
			APLOG_ERROR << it.first << " does not correspond to an antenna pid.";
			continue;
		}
		if (!params.configure(it.second))
		{
			APLOG_ERROR << it.first << " configuration not valid.";
			continue;
		}

		std::string name;
		auto map = PanTiltPIDBimapLeft.find((PanTiltPIDs) (int) pid->second); //TODO Check Airplane or Helicopter
		if (map == PanTiltPIDBimapLeft.end())
		{
			name = "Invalid";
		}
		else
		{
			name = map->second;
		}
		PIDInfo info(name, params);
		tunePID((int) pid->second, params);
		pidParams_.insert(std::make_pair((int) pid->second, info));
	}
	return true;
}

bool
PanTiltPIDCascade::tunePID(int pidIndicator, const Control::PID::Parameters& params)
{
	PanTiltPIDs pid = static_cast<PanTiltPIDs>(pidIndicator);

	switch (pid)
	{
	case PanTiltPIDs::HEADING:
		headingPID_->setControlParameters(params);
		break;
	case PanTiltPIDs::HEADINGRATE:
		headingRatePID_->setControlParameters(params);
		break;
	case PanTiltPIDs::PITCH:
		pitchPID_->setControlParameters(params);
		break;
	case PanTiltPIDs::PITCHRATE:
		pitchRatePID_->setControlParameters(params);
		break;
	default:
		APLOG_WARN << "Unknown pidIndicator. Ignore.";
		return false;
	}
	return true;
}

bool
PanTiltPIDCascade::tuneRollBounds(double, double)
{
	return true;
}

bool
PanTiltPIDCascade::tunePitchBounds(double, double)
{
	return true;
}

std::map<int, PIDStatus>
PanTiltPIDCascade::getPIDStatus()
{
	std::map<int, PIDStatus> status;
	status.insert(std::make_pair((int) PanTiltPIDs::HEADING, headingPID_->getStatus()));
	status.insert(std::make_pair((int) PanTiltPIDs::PITCH, pitchPID_->getStatus()));
	status.insert(std::make_pair((int) PanTiltPIDs::HEADINGRATE, headingRatePID_->getStatus()));
	status.insert(std::make_pair((int) PanTiltPIDs::PITCHRATE, pitchRatePID_->getStatus()));
	return status;
}

void
PanTiltPIDCascade::evaluate()
{
	double x = aircraftSensorData->position.x - panTiltSensorData->gpsData.position[0];
	double y = aircraftSensorData->position.y - panTiltSensorData->gpsData.position[1];
	double r = sqrt(x * x + y * y);
	double vr = (x * aircraftSensorData->velocity.linear.x
			+ y * aircraftSensorData->velocity.linear.y) / r;
	headingDeviation_ = antennaTarget->heading - panTiltSensorData->imuData.attitude[2]
			+ antennaTarget->headingoffset; //IMU mounted sideways so adding offset
	if (headingDeviation_ > M_PI)
	{
		headingDeviation_ -= 2 * M_PI;
	}
	else if (headingDeviation_ < -M_PI)
	{
		headingDeviation_ += 2 * M_PI;
	}
	pitchDeviation_ = antennaTarget->pitch + antennaTarget->pitchoffset
			- panTiltSensorData->imuData.attitude[0]; //IMU mounted sideways so adding offset
	if (pitchDeviation_ > M_PI)
	{
		pitchDeviation_ -= 2 * M_PI;
	}
	else if (pitchDeviation_ < -M_PI)
	{
		pitchDeviation_ += 2 * M_PI;
	}
	psiDot_ =
			(x * aircraftSensorData->velocity.linear.x - y * aircraftSensorData->velocity.linear.y)
					/ (x * x + y * y);
	thetaDot_ = (r * aircraftSensorData->velocity.linear.z - aircraftSensorData->position.z * vr)
			/ (r * r);
	controlEnv_.evaluate();
}

const PIDParametersMap&
PanTiltPIDCascade::getPIDParamMap() const
{
	return pidParams_;
}
