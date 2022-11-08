// created by Mark O. Brown
#pragma once

#include "ParameterSystem/ParameterSystemStructures.h"
#include <string>
#include <vector>
#include <AnalogInput/calInfo.h>


class Expression {
	public:
		Expression( );
		Expression( std::string expressionString );
		void assertValid( std::vector<parameterType>& variables, std::string scope );
		// default values are empty objects and variation #-1.
		static std::vector<std::string> splitString( std::string workingString );
		std::string expressionStr;
		std::string calName = "";
		bool varies( );
		// overloading evaluate to achieve
		//	double evaluate ( std::vector<parameterType>& variables, unsigned variation = -1,
		//		std::vector<calResult>& calibrations = std::vector<calResult>()); where the last arg is invalid since reference function argument
		//  default value need to be a lvalue or it need to be const
		double evaluate(std::vector<parameterType>& variables);
		double evaluate(std::vector<parameterType>& variables, unsigned variation);
		double evaluate(std::vector<parameterType>& variables, unsigned variation, 
			std::vector<calResult>& calibrations);
		// Overload to take the default argument of std::vector<parameterType>& 
		double evaluate();
		void internalEvaluate();
		void internalEvaluate(std::vector<parameterType>& variables, unsigned totalVariationNumber = -1);
		double handleCalibration (double val, std::vector<calResult>& calibrations);
		double getValue ( unsigned variation );	
	private:
		void doMultAndDiv( std::vector<std::string>& terms );
		void doAddAndSub( std::vector<std::string>& terms );
		double reduce( std::vector<std::string> terms );
		void evaluateFunctions( std::vector<std::string>& terms );
		bool expressionVaries = false;
		std::string expressionScope;
		std::vector<double> values;
};