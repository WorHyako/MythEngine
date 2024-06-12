#pragma once
#include <iostream>

#include "mini/ini.h"

#define GET_STRUCT_VARIABLE_NAME(Struct, Variable) (void(&Struct.Variable),#Variable)
#define GET_VARIABLE_NAME(Variable) (void(Variable),#Variable)

class Config
{
public:
	Config(const std::string& Path)
		: ConfigFile(Path)
	{
		mINI::INIStructure ini;
		bool readSuccess = ConfigFile.read(ini);
		if(readSuccess == false)
		{
			std::cerr << "\n * **Error: Could not open config file for reading\n";
		}
		ConfigData = ini;
	}
	~Config(){}

	bool IsValid() const
	{
		return ConfigData.size() > 0;
	}

	mINI::INIStructure GetConfigData() const
	{
		return ConfigData;
	}

	template <typename T> T convert_to(const std::string& str)
	{
		std::istringstream ss(str);
		T num;
		ss >> num;
		return num;
	}

	template<typename T>
	void ReadValueSafety(const std::string& Section, const std::string& Key, T& Value)
	{
		Value = convert_to<T>(ConfigData.get(Section).get(Key));
	}

	template<typename T>
	void WriteValueSafety(const std::string& Section, const std::string& Key, T Value)
	{
		if(ConfigData.has(Section) && ConfigData[Section].has(Key))
		{
			ConfigData[Section][Key] = std::to_string(Value);
		}
	}

	void WriteConfigFile()
	{
		if(IsValid())
		{
			ConfigFile.write(ConfigData);
		}
	}

private:
	mINI::INIFile ConfigFile;
	mINI::INIStructure ConfigData;
};
