#pragma once
#include <gsl/gsl_bspline.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_statistics.h>
#include <vector>
#include <utility>
#include <cmath>

class BSplineFit
{
public:
	BSplineFit(const BSplineFit&);
	BSplineFit();
	~BSplineFit();
	void initialize(unsigned dataSize, std::vector<double> datax, std::vector<double> datay,
		unsigned orderBSpline, unsigned nBreak);
	void freeAll();
	void solve_system();
	double calculateY(double x);
	std::vector<double> calculateY(std::vector<double> x);

	std::vector<double> getConfidentInterval(double level = 0.95);
	std::vector<double> getBoudaryPolyCoef(double boundaryPos);
	std::vector<double> getfittedPara();
	//return in the order of {orderBSpline, dataSize, nBreak, nBasis}
	std::vector<unsigned> getBSplinePara();
	std::pair<double, double> getxlim();

	static std::vector<double> calculateY(unsigned orderBSpline, unsigned nBreak, 
		std::pair<double, double> xlim, std::vector<double> coef, std::vector<double> xpts);

private:


private:
	bool emptyStart;
	std::vector<double> datax, datay; // datax should be sorted in the ascending order

	gsl_matrix* X; // design matrix in linear least square regression, dataSize x nBasis, each row is evaluated for the same x with different basis function
	gsl_vector* y; // datay for fitting
	gsl_vector* coef; // coefficient in front of basis function
	gsl_matrix* cov; // variance-covariance matrix of linear regression, cov = RSS/(dataSize-nBasis) * (X^T*X)^(-1), RSS is called chisq in GSL
					 // where RSS is the sum of square residual = sum(yi-\hat{yi})^2, RSS/(dataSize-nBasis) = s^2, the unbiased estimator of system variance
	
	gsl_vector* B; // Bspline vector for a particular point x, to facilitate eval inside functions
	double RSS, TSS, Rsq;
	std::vector<double> confi95;
	std::vector<double> leftPoly, rightPoly;


	gsl_bspline_workspace* bw;
	gsl_multifit_linear_workspace* mw;
	unsigned orderBSpline; // order of B spline, gives a polynomial of order = orderBSpline-1. The common case of cibic B-splines is given by k = 4
	unsigned dataSize; // number of data points to be fit
	unsigned nBreak; // number of break points, including left and right end points
	unsigned nBasis; // number of basis function in the interval = nbreak - 2 + orderBSpline
	// note this is the same as the number of parameter in the final linear least square fitting
	// when orderBSpline = 1, nBasis = nBreak-1, i.e. nBreak-1 step function fill
	// in each interval. When orderBSpline increase, the basis function number also increase
	// since the basis function is now extending over several interval and the left/right boundary will
	// see tails of basis function whose center point is outside the total range
};

