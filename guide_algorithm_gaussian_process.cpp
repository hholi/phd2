//
//  guide_gaussian_process.cpp
//  PHD
//
//  Created by Stephan Wenninger
//  Copyright 2014, Max Planck Society.

/*
*  This source code is distributed under the following "BSD" license
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*    Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*    Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*    Neither the name of Bret McKee, Dad Dog Development, nor the names of its
*     Craig Stark, Stark Labs nor the names of its
*     contributors may be used to endorse or promote products derived from
*     this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*/

#include "phd.h"

#include "UDPGuidingInteraction.h"

#include "guide_algorithm_gaussian_process.h"
#include <wx/stopwatch.h>


#include "math_tools.h" 

// Should be removed
//#include "circular_buffer.h"

#include "math_tools.h"
#include "gaussian_process.h"
#include "covariance_functions.h"


class GuideGaussianProcess::GuideGaussianProcessDialogPane : public ConfigDialogPane
{
    GuideGaussianProcess *m_pGuideAlgorithm;
    wxSpinCtrlDouble *m_pControlGain;
    wxSpinCtrl       *m_pNbMeasurementMin;
    wxSpinCtrl       *m_pNbPointsOptimisation;

    wxSpinCtrlDouble *m_pHyperDiracNoise;
    wxSpinCtrlDouble *m_pPKLengthScale;
    wxSpinCtrlDouble *m_pPKPeriodLength;
    wxSpinCtrlDouble *m_pPKSignalVariance;
    wxSpinCtrlDouble *m_pSEKLengthScale;

public:
    GuideGaussianProcessDialogPane(wxWindow *pParent, GuideGaussianProcess *pGuideAlgorithm)
      : ConfigDialogPane(_("Gaussian Process Guide Algorithm"),pParent)
    {
        m_pGuideAlgorithm = pGuideAlgorithm;

        int width = StringWidth(_T("000.00"));
        m_pControlGain = new wxSpinCtrlDouble(pParent, wxID_ANY, _T("foo2"),
                                              wxDefaultPosition,wxSize(width+30, -1),
                                              wxSP_ARROW_KEYS, 0.0, 1.0, 0.0, 0.05,
                                              _T("Control Gain"));
        m_pControlGain->SetDigits(2);

        // nb elements before starting the inference
        m_pNbMeasurementMin = new wxSpinCtrl(pParent, wxID_ANY, _T("foo2"),
                                             wxDefaultPosition,wxSize(width+30, -1),
                                             wxSP_ARROW_KEYS, 0, 100, 0, 
                                             _T("Min nb meas."));

        // hyperparameters
        m_pHyperDiracNoise = new wxSpinCtrlDouble(pParent, wxID_ANY, _T("foo2"),
                                                  wxDefaultPosition,wxSize(width+30, -1),
                                                  wxSP_ARROW_KEYS, 0.0, 50, 0.0, 0.01);
        m_pHyperDiracNoise->SetDigits(2);


        m_pPKLengthScale = new wxSpinCtrlDouble(pParent, wxID_ANY, _T("foo2"),
                                                wxDefaultPosition,wxSize(width+30, -1),
                                                wxSP_ARROW_KEYS, 0.0, 600, 0.0, 0.01);
        m_pPKLengthScale->SetDigits(2);



        m_pPKPeriodLength = new wxSpinCtrlDouble(pParent, wxID_ANY, _T("foo2"),
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 0.0, 600, 0.0, 0.01);
        m_pPKPeriodLength->SetDigits(2);

        
        m_pPKSignalVariance = new wxSpinCtrlDouble(pParent, wxID_ANY, _T("foo2"),
                                                   wxDefaultPosition,wxSize(width+30, -1),
                                                   wxSP_ARROW_KEYS, 0.0, 600, 0.0, 0.01);
        m_pPKSignalVariance->SetDigits(2);

        m_pSEKLengthScale = new wxSpinCtrlDouble(pParent, wxID_ANY, _T("foo2"),
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 0.0, 600, 0.0, 0.01);
        m_pSEKLengthScale->SetDigits(2);

        // nb points between consecutive calls to the optimisation
        m_pNbPointsOptimisation = new wxSpinCtrl(pParent, wxID_ANY, _T("foo2"),
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 0, 100, 0, 
                                                 _T("Optim. every"));


        //
        // TODO Add description of the control gain!
        //
        DoAdd(_("Control Gain"), m_pControlGain,
              _("Control gain. Default = 1.0"));

        DoAdd(_("Nb data min"), m_pNbMeasurementMin,
              _("Minimal number of measurements to start the inference."));

        DoAdd(_("Nb points optimisation"), m_pNbPointsOptimisation,
              _("Number of points between two consecutive calls to the optimisation. \"0\" disables the optimisation completely"));

        // hyperparameters
        DoAdd(_("Dirac noise"), m_pHyperDiracNoise,
              _("Noise associated to the diract kernel."));
        DoAdd(_("Length scale [PER]"), m_pPKLengthScale,
              _("Length scale of the periodic kernel (l_P)."));
        DoAdd(_("Period [PER]"), m_pPKPeriodLength,
              _("Period length of the periodic kernel (lambda)."));
        DoAdd(_("Signal variance [PER]"), m_pPKSignalVariance,
              _("Signal variance of the periodic kernel (theta)."));
        DoAdd(_("Length scale [SE]"), m_pSEKLengthScale,
              _("Length scale of the square exponetional kernel (l_SE)."));
    }

    virtual ~GuideGaussianProcessDialogPane(void) 
    {
      // no need to destroy the widgets, this is done by the parent...
    }

    /* Fill the GUI with the parameters that are currently chosen in the
      * guiding algorithm
      */
    virtual void LoadValues(void)
    {
      m_pControlGain->SetValue(m_pGuideAlgorithm->GetControlGain());
      m_pNbMeasurementMin->SetValue(m_pGuideAlgorithm->GetNbMeasurementsMin());
      m_pNbPointsOptimisation->SetValue(m_pGuideAlgorithm->GetNbPointsBetweenOptimisation());

      std::vector<double> hyperparameters = m_pGuideAlgorithm->GetGPHyperparameters();
      assert(hyperparameters.size() == 5);

      m_pHyperDiracNoise->SetValue(hyperparameters[0]);
      m_pPKLengthScale->SetValue(hyperparameters[1]);
      m_pPKPeriodLength->SetValue(hyperparameters[2]);
      m_pPKSignalVariance->SetValue(hyperparameters[3]);
      m_pSEKLengthScale->SetValue(hyperparameters[4]);
    }

    // Set the parameters chosen in the GUI in the actual guiding algorithm
    virtual void UnloadValues(void)
    {
      m_pGuideAlgorithm->SetControlGain(m_pControlGain->GetValue());
      m_pGuideAlgorithm->SetNbElementForInference(m_pNbMeasurementMin->GetValue());
      m_pGuideAlgorithm->SetNbPointsBetweenOptimisation(m_pNbPointsOptimisation->GetValue());

      std::vector<double> hyperparameters(5);

      hyperparameters[0] = m_pHyperDiracNoise->GetValue();
      hyperparameters[1] = m_pPKLengthScale->GetValue();
      hyperparameters[2] = m_pPKPeriodLength->GetValue();
      hyperparameters[3] = m_pPKSignalVariance->GetValue();
      hyperparameters[4] = m_pSEKLengthScale->GetValue();

      m_pGuideAlgorithm->SetGPHyperparameters(hyperparameters);
    }

};


struct gp_guiding_circular_datapoints
{
  double timestamp;
  double measurement;
  double modified_measurements;
};


// parameters of the GP guiding algorithm
struct GuideGaussianProcess::gp_guide_parameters
{
    typedef gp_guiding_circular_datapoints data_points;
    circular_buffer<data_points> circular_buffer_parameters;

    wxStopWatch timer_;
    double control_signal_;
    double control_gain_;
    double elapsed_time_ms_;

    int min_nb_element_for_inference;
    int min_points_for_optimisation;


    covariance_functions::PeriodicSquareExponential covariance_function_;
    GP gp_;

    gp_guide_parameters() :
      circular_buffer_parameters(100),
      timer_(),
      control_signal_(0.0),
      elapsed_time_ms_(0.0),
      min_nb_element_for_inference(0),
      min_points_for_optimisation(0),
      gp_(covariance_function_)
    {

    }

    data_points& get_last_point() 
    {
      return circular_buffer_parameters[circular_buffer_parameters.size() - 1];
    }

    data_points& get_second_last_point() 
    {
      return circular_buffer_parameters[circular_buffer_parameters.size() - 2];
    }

    size_t get_number_of_measurements() const
    {
      return circular_buffer_parameters.size();
    }

    void add_one_point()
    {
      circular_buffer_parameters.push_front(data_points());
    }

    void clear()
    {
      circular_buffer_parameters.clear();
      gp_.clear();
    }

};




static const double DefaultControlGain = 1.0;           // control gain
static const int    DefaultNbMinPointsForInference = 5; // minimal number of points for doing the inference

static const double DefaultGaussianNoiseHyperparameter = std::sqrt(2)*0.55*0.2; // default Gaussian process noise
static const double DefaultLengthScalePerKer           = 5.234;                 // length-scale of the periodic kernel
static const double DefaultPeriodLengthPerKer          = 396;                   // P_p, period-length of the periodic kernel
static const double DefaultSignalVariancePerKer        = 0.355;                 // signal-variance of the periodic kernel
static const double DefaultLengthScaleSEKer            = 200;                   // length-scale of the SE-kernel

static const int    DefaultNbPointsBetweenOptimisation = 10;                    // number of points collected between two consecutive calls to the optimisation
                                                                                // 0 indicates no optimisation.


GuideGaussianProcess::GuideGaussianProcess(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis),
      udpInteraction_("localhost", "1308", "1309"),
      parameters(0)
{
    parameters = new gp_guide_parameters();
    wxString configPath = GetConfigPath();
    
    double control_gain = pConfig->Profile.GetDouble(configPath + "/gp_controlGain", DefaultControlGain);
    SetControlGain(control_gain);


    int nb_element_for_inference = pConfig->Profile.GetInt(configPath + "/gp_nbminelementforinference", DefaultNbMinPointsForInference);
    SetNbElementForInference(nb_element_for_inference);

    int nb_points_between_optimisation = pConfig->Profile.GetInt(configPath + "/gp_nbpointsbetweenoptimisations", DefaultNbPointsBetweenOptimisation);
    SetNbPointsBetweenOptimisation(nb_points_between_optimisation);

    std::vector<double> v_hyperparameters(5);
    v_hyperparameters[0] = pConfig->Profile.GetDouble(configPath + "/gp_gaussian_noise",        DefaultGaussianNoiseHyperparameter);
    v_hyperparameters[1] = pConfig->Profile.GetDouble(configPath + "/gp_length_scale_per_kern", DefaultLengthScalePerKer);
    v_hyperparameters[2] = pConfig->Profile.GetDouble(configPath + "/gp_period_per_kern",       DefaultPeriodLengthPerKer);
    v_hyperparameters[3] = pConfig->Profile.GetDouble(configPath + "/gp_sigvar_per_kern",       DefaultSignalVariancePerKer);
    v_hyperparameters[4] = pConfig->Profile.GetDouble(configPath + "/gp_length_scale_se_kern",  DefaultLengthScaleSEKer);

    SetGPHyperparameters(v_hyperparameters);

    reset();
}

GuideGaussianProcess::~GuideGaussianProcess(void)
{
    delete parameters;
}


ConfigDialogPane *GuideGaussianProcess::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuideGaussianProcessDialogPane(pParent, this);
}



bool GuideGaussianProcess::SetControlGain(double control_gain)
{
    bool error = false;

    try 
    {
        if (control_gain < 0) 
        {
            throw ERROR_INFO("invalid controlGain");
        }

        parameters->control_gain_ = control_gain;
    }
    catch (wxString Msg) 
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->control_gain_ = DefaultControlGain;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_controlGain", parameters->control_gain_);

    return error;
}

bool GuideGaussianProcess::SetNbElementForInference(int nb_elements)
{
    bool error = false;

    try 
    {
        if (nb_elements < 0) 
        {
            throw ERROR_INFO("invalid number of elements");
        }

        parameters->min_nb_element_for_inference = nb_elements;
    }
    catch (wxString Msg) 
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->min_nb_element_for_inference = DefaultNbMinPointsForInference;
    }

    pConfig->Profile.SetInt(GetConfigPath() + "/gp_nbminelementforinference", parameters->min_nb_element_for_inference);

    return error;
}

bool GuideGaussianProcess::SetNbPointsBetweenOptimisation(int nb_points)
{
    bool error = false;

    try 
    {
        if (nb_points < 0) 
        {
            throw ERROR_INFO("invalid number of points");
        }

        parameters->min_points_for_optimisation = nb_points;
    }
    catch (wxString Msg) 
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->min_points_for_optimisation = DefaultNbPointsBetweenOptimisation;
    }

    pConfig->Profile.SetInt(GetConfigPath() + "/gp_nbpointsbetweenoptimisations", parameters->min_points_for_optimisation);

    return error;
}

bool GuideGaussianProcess::SetGPHyperparameters(std::vector<double> const &hyperparameters)
{
    if(hyperparameters.size() != 5)
        return false;

    Eigen::VectorXd hyperparameters_eig = Eigen::VectorXd::Map(&hyperparameters[0], hyperparameters.size());
    bool error = false;


    // we do this check in sequence: maybe there would be additional checks on this later.

    // gaussian process noise (dirac kernel)
    try 
    {
        if (hyperparameters_eig(0) < 0) 
        {
            throw ERROR_INFO("invalid noise for dirac kernel");
        }
    }
    catch (wxString Msg) 
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters_eig(0) = DefaultGaussianNoiseHyperparameter;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_gaussian_noise", hyperparameters_eig(0));


    // length scale periodic kernel
    try 
    {
        if (hyperparameters_eig(1) < 0) 
        {
            throw ERROR_INFO("invalid length scale for periodic kernel");
        }
    }
    catch (wxString Msg) 
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters_eig(1) = DefaultLengthScalePerKer;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_length_scale_per_kern", hyperparameters_eig(1));


    // period length periodic kernel
    try 
    {
        if (hyperparameters_eig(2) < 0) 
        {
            throw ERROR_INFO("invalid period length for periodic kernel");
        }
    }
    catch (wxString Msg) 
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters_eig(2) = DefaultPeriodLengthPerKer;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_period_per_kern", hyperparameters_eig(2));


    // signal variance periodic kernel
    try 
    {
        if (hyperparameters_eig(3) < 0) 
        {
            throw ERROR_INFO("invalid signal variance for the periodic kernel");
        }
    }
    catch (wxString Msg) 
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters_eig(3) = DefaultSignalVariancePerKer;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_sigvar_per_kern", hyperparameters_eig(3));


    // length scale SE kernel
    try 
    {
        if (hyperparameters_eig(4) < 0) 
        {
            throw ERROR_INFO("invalid length scale for SE kernel");
        }
    }
    catch (wxString Msg) 
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters_eig(4) = DefaultLengthScaleSEKer;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_length_scale_se_kern", hyperparameters_eig(4));
  
    parameters->gp_.setHyperParameters(hyperparameters_eig);
    return error;
}




double GuideGaussianProcess::GetControlGain() const
{
    return parameters->control_gain_;
}

int GuideGaussianProcess::GetNbMeasurementsMin() const
{
    return parameters->min_nb_element_for_inference;
}

int GuideGaussianProcess::GetNbPointsBetweenOptimisation() const
{
    return parameters->min_points_for_optimisation;
}


std::vector<double> GuideGaussianProcess::GetGPHyperparameters() const
{
    Eigen::VectorXd hyperparameters = parameters->gp_.getHyperParameters();
    return std::vector<double>(hyperparameters.data(),
                               hyperparameters.data() + 5);
}



wxString GuideGaussianProcess::GetSettingsSummary()
{
    static const char* format = 
      "Control Gain = %.3f\n"
      "Hyperparameters\n"
      "\tGP noise = %.3f\n"
      "\tLength scale periodic kern = %.3f\n"
      "\tPeriod Length periodic kern = %.3f\n"
      "\tSignal-variance periodic kern = %.3f\n"
      "\tLength scale SE kern = %.3f\n"
      "Optimisation called every = %.3d points\n"
    ;

    Eigen::VectorXd hyperparameters = parameters->gp_.getHyperParameters();

    return wxString::Format(
      format, 
      GetControlGain(), 
      hyperparameters(0), hyperparameters(1), 
      hyperparameters(2), hyperparameters(3), 
      hyperparameters(4),
      parameters->min_points_for_optimisation);
}



GUIDE_ALGORITHM GuideGaussianProcess::Algorithm(void)
{
    return GUIDE_ALGORITHM_GAUSSIAN_PROCESS;
}

void GuideGaussianProcess::HandleTimestamps()
{
    if (parameters->get_number_of_measurements() == 0)
    {
        parameters->timer_.Start();
    }
    double time_now = parameters->timer_.Time();
    double delta_measurement_time_ms = time_now - parameters->elapsed_time_ms_;
    parameters->elapsed_time_ms_ = time_now;
    parameters->get_last_point().timestamp = parameters->elapsed_time_ms_ - delta_measurement_time_ms / 2;
}

void GuideGaussianProcess::HandleMeasurements(double input)
{
    parameters->add_one_point();
    parameters->get_last_point().measurement = input;
}

void GuideGaussianProcess::HandleModifiedMeasurements(double input)
{
    // If there is no previous measurement, a random one is generated.
    if(parameters->get_number_of_measurements() <= 1)
    {
        //The daytime indoor measurement noise SD is 0.25-0.35
        const double indoor_noise_standard_deviation = 0.25;
        double first_random_measurement = indoor_noise_standard_deviation *
            math_tools::generate_normal_random_double();
        double new_modified_measurement =
            parameters->control_signal_ +
            first_random_measurement * (1 - parameters->control_gain_) -
            input;
        parameters->get_last_point().modified_measurements = new_modified_measurement;
    }
    else
    {
        gp_guide_parameters::data_points &last_point = parameters->get_last_point();
        double new_modified_measurement =
            parameters->control_signal_ +
            parameters->get_second_last_point().measurement * (1 - parameters->control_gain_) -
            last_point.measurement;
        last_point.modified_measurements = new_modified_measurement;
    }
}

double GuideGaussianProcess::result(double input)
{
    HandleMeasurements(input);
    HandleTimestamps();
    HandleModifiedMeasurements(input);

    /*
     * Need to read this value here because it is not loaded at the construction
     * time of this object.
     * 
     *
     * Not used when testing the Code with Matlab, will be used in the actual GP
     */
    double delta_controller_time_ms = pFrame->RequestedExposureDuration();



    // This is the Code sending the circular buffers to Matlab:

#if 1

    Eigen::VectorXd timestamps(parameters->get_number_of_measurements());
    Eigen::VectorXd measurements(parameters->get_number_of_measurements());

    for (size_t i = 0; i < parameters->get_number_of_measurements(); i++)
    {
        timestamps(i) = parameters->circular_buffer_parameters[i].timestamp;
        measurements(i) = parameters->circular_buffer_parameters[i].measurement;
    }

    double* timestamp_data = timestamps.data();
    double* modified_measurement_data = measurements.data();
    double result;
    double wait_time = 100;

    bool sent = false;
    bool received = false;


    // Send the input
    double input_buf[] = { input };
    sent = udpInteraction_.SendToUDPPort(input_buf, 8);
    received = udpInteraction_.ReceiveFromUDPPort(&result, 8);
    wxMilliSleep(wait_time);

    // Send the size of the buffer
    double size = timestamps.size();
    double size_buf[] = { size };
    sent = udpInteraction_.SendToUDPPort(size_buf, 8);
    received = udpInteraction_.ReceiveFromUDPPort(&result, 8);
    wxMilliSleep(wait_time);

    // Send modified measurements
    sent = udpInteraction_.SendToUDPPort(modified_measurement_data, size * 8);
    received = udpInteraction_.ReceiveFromUDPPort(&result, 8);
    wxMilliSleep(wait_time);

    // Send timestamps
    sent = udpInteraction_.SendToUDPPort(timestamp_data, size * 8);
    // Receive the final control signal
    received = udpInteraction_.ReceiveFromUDPPort(&result, 8);

    return result;
#endif


    /*
     * This is the code running the actual GP
     *
     * TODO: 
     * - Let GP class be a member of this Guiding Class
     * - Call the GP´s BFGS Optimizer every once in a while (how often?)
     *
     */


    if (parameters->get_number_of_measurements() > parameters->min_nb_element_for_inference)
    {
        Eigen::VectorXd timestamps(parameters->get_number_of_measurements());
        Eigen::VectorXd measurements(parameters->get_number_of_measurements());

        for(size_t i = 0; i < parameters->get_number_of_measurements(); i++)
        {
          timestamps(i) = parameters->circular_buffer_parameters[i].timestamp;
          measurements(i) = parameters->circular_buffer_parameters[i].measurement;
        }

        // inference of the GP with this new points
        parameters->gp_.infer(
          timestamps,
          measurements);

        // prediction of the next
        Eigen::VectorXd next_location(1);
        next_location << parameters->elapsed_time_ms_ + delta_controller_time_ms / 2;
        Eigen::VectorXd prediction = parameters->gp_.predict(next_location).first;
        
        parameters->control_signal_ = prediction(0) - parameters->control_gain_ * input / delta_controller_time_ms;

    }
    else
    {
        // Simpler prediction when there are not enough data points for the GP
        parameters->control_signal_ = -input / delta_controller_time_ms;
    }


    // here from time to time launch the optimiser

    if(parameters->min_points_for_optimisation)
    {
        // performing the optimisation every x steps
    
    
    }


    return parameters->control_signal_;
}


void GuideGaussianProcess::reset()
{
    parameters->clear();
    return;
}