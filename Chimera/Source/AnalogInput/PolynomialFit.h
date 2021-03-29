#pragma once
#include <gsl/gsl_multifit_nlinear.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_cdf.h>

#include <vector>
#include <functional>
#include <algorithm>
#include <tuple>

// this "fdf_Helper" helper class is used for create the polynomial to be fit in the runtime. 
// Since GSL would like a static function, have to do some trick to make it happy
// see https://stackoverflow.com/questions/13074756/how-to-avoid-static-member-function-when-using-gsl-with-c?noredirect=1&lq=1
// https://stackoverflow.com/questions/6610046/stdfunction-and-stdbind-what-are-they-and-when-should-they-be-used
// https://stackoverflow.com/questions/7582546/using-generic-stdfunction-objects-with-member-functions-in-one-class

class PolynomialFit
{
public:
    struct data
    {
        double* x;
        double* y;
        size_t n;
        std::function<int(const gsl_vector* p, void* datafit, gsl_vector* f)> f = NULL;
        std::function<int(const gsl_vector* p, void* datafit, gsl_matrix* J)> df = NULL;
        std::function<int(const gsl_vector* p, const gsl_vector* v, void* datafit, gsl_vector* fvv)> fvv = NULL;
    };
    class fdf_Helper
    {
    public:
        static int invoke_f(const gsl_vector* p, void* datafit, gsl_vector* f) {
            PolynomialFit::data* dataf = static_cast<PolynomialFit::data*>(datafit);
            return dataf->f(p, datafit, f);
        }
        static int invoke_df(const gsl_vector* p, void* datafit, gsl_matrix* J) {
            PolynomialFit::data* dataf = static_cast<PolynomialFit::data*>(datafit);
            return dataf->df(p, datafit, J);
        }
        static int invoke_fvv(const gsl_vector* p, const gsl_vector* v, void* datafit, gsl_vector* fvv) {
            PolynomialFit::data* dataf = static_cast<PolynomialFit::data*>(datafit);
            return dataf->fvv(p, v, datafit, fvv);
        }
    };

private:

    gsl_vector* p0;     /* initial fitting parameters: a0,...,an, (b0,b1)*/
    gsl_vector* p;      /* fitting parameters */
    gsl_vector* f;      /* residual yi-y(xi) */
    gsl_matrix* covar;  /*covariance matrix, not yet multiplied by sigma, so is not variance-covariance matrix*/

    gsl_multifit_nlinear_fdf         fdf;
    gsl_multifit_nlinear_parameters  fdf_params;
    gsl_multifit_nlinear_workspace*  work;

    data                             fit_data;

    size_t                           max_iter;
    double                           ptol; /* tolerance on fitting parameter p */
    double                           gtol; /* tolerance on gradient */
    double                           ftol;

    int                              info; /* fiting stop reason, for debug*/
    double                           rss;  /* mean of sum of residual square, dof corrected*/
    gsl_vector*                      confid95;

public:

    /* model function: a0 + a1*x + a2*x^2 +...+ b0*sqrt(x-b1) */

    PolynomialFit() {};
    PolynomialFit(size_t n, double* datax, double* datay, unsigned order, bool useSqrt,
        std::vector<double> init_para,
        size_t max_iter = 200, double ptol = 1.0e-8, double gtol = 1.0e-8, double ftol = 1.0e-8);
    double polynomial(std::vector<double> para, double t);
    ~PolynomialFit();
    void solve_system();
    void set_data(double* datay);
    void set_data(size_t n, double* datax, double* datay, bool free = true);
    void set_initialP(std::vector<double> para);

    std::vector<double> calcFittedGaussian();
    std::vector<double> fittedPara() const;
    std::vector<double> confidence95Interval() const;


private:

    bool useSqrt;
    unsigned order;
    unsigned numOfPara;

    void init_dataFunc();

    //static int funcf_Wrapper/*next line is the argument of this "funcf_Wrapper"*/
    //(const gsl_vector* p, void* datafit, gsl_vector* f);

    //static int (*funcf_Wrapper/*next line is the argument of this "funcf_Wrapper"*/
    //(std::function<int(const gsl_vector* p, void* datafit, gsl_vector* f)> func_f))
    //    (const gsl_vector* p, void* datafit, gsl_vector* f);

    int func_f(const gsl_vector* p, void* datafit, gsl_vector* f);
    int func_df(const gsl_vector* p, void* datafit, gsl_matrix* J);
    int func_fvv(const gsl_vector* p, const gsl_vector* v, void* datafit, gsl_vector* fvv);
    



};


