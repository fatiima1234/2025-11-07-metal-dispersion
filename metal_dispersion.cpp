// Metal Dispersion Analyzer
// Author: Fatemeh HadavandMirzaee
// Version 2.0
// Date: 3 January 2026
// Description: A C++ program for base dispersion analysis of metals (e.g., Ag, Au).
// Loads wavelength-dependent refractive index (n) and extinction coefficient (k)
// data from a file, computes the complex permittivity ε = ε₁ + iε₂ where
// ε₁ = n² − k² and ε₂ = 2nk. This version introduces energy-domain permittivity analysis and Drude model fitting.
// Basic wavelength-domain plots can be enabled for data validation and educational purposes,
// while advanced plots focus on physical modeling and comparison with experiment.

#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

constexpr double c = 2.99792458e8;
constexpr double pi = 3.1415;

// Default high-frequency permittivity
constexpr double eps_inf = 4.3; 

// Drude model is valid only below interband transition energies.
// We restrict fitting to omega_min < ω < omega_max to avoid interband effects.
constexpr double omega_min = 1.5e15;   // rad/s
constexpr double omega_max = 4.0e15;   // rad/s

// Parameter search ranges
constexpr double omega_p_min = 1.0e15; // rad/s
constexpr double omega_p_max = 3.0e16; // rad/s
constexpr double gamma_min   = 1.0e13; // s^-1
constexpr double gamma_max   = 1.5e14; // s^-1

// Step sizes (grid resolution)
double domega_p = 0.05e15;
double dgamma   = 0.1e13;


// Basic wavelength-domain plots can be enabled for data validation and educational purposes.
// Advanced plots focus on physical modeling and comparison with experiment. This feature is tunable in the main function.
enum class PlotLevel {
    Basic,      // v1-style plots (n, k, ε vs wavelength)
    Advanced    // v2 plots (ε vs energy + Drude fit)
};

class Material {
public:
    Material(const std::string& name){
        name_ = name;
    }

    std::string getName() const {
        return name_;
    }

    std::vector<double> getWavelength() const {
        return wavelength_;
    }
    
    std::vector<double> getN() const {
        return n_;
    }
    
    std::vector<double> getK() const {
        return k_;
    }

    std::vector<double> getOmega() const {
        std::vector<double> omega;
        for (size_t i = 0; i < wavelength_.size(); i++){
            omega.push_back(
                (2*pi*c)/(wavelength_[i]*1e-9)
            );
        }
        return omega;
    }

    std::vector<double> getEnergy() const {
        std::vector<double> energy;
        for (size_t i = 0; i < wavelength_.size(); i++){
            energy.push_back(
                (1240)/(wavelength_[i])
            );
        }
        return energy;
    }

    // Loading data
    void loadData (const std::string& filename){
        std::ifstream file (filename);
        if (!file.is_open()) {
            throw std::runtime_error("Error: Could not open file " + filename);
        }
        double lambda, n_real, n_imag;
        char comma;
        while(file >> lambda >> comma >> n_real >> comma >> n_imag){
            wavelength_.push_back(lambda);
            n_.push_back(n_real);
            k_.push_back(n_imag);
        }
        std::cout << "\n***Imported data information***\nLoaded "<< wavelength_.size() << " data points for " << name_ << " between " << wavelength_.front() << " and " << wavelength_.back() << " nm\n"; 
        }

    // Computes and returns (ε₁, ε₂) from loaded n and k data.
    std::pair<std::vector<double>, std::vector<double>> computeEpsilon() const {
        std::vector<double> e_real, e_imag;
        for (size_t i = 0; i < wavelength_.size(); ++i){
            double eps_real = n_[i] * n_[i] - k_[i] * k_[i];
            double eps_imag = 2 * n_[i] * k_[i];
            e_real.push_back(eps_real);
            e_imag.push_back(eps_imag);
        }
        return std::make_pair(e_real, e_imag);
    }

private:
    std::string name_;
    std::vector<double> wavelength_;
    std::vector<double> n_;
    std::vector<double> k_;
};

class Plot {
public:

    // Plots two datasets (i.e. real and imaginary parts of refractive index) against x axis (i.e. wavelength).
    static void dispersionPlot(const int number,
                               const std::vector<double>& x,
                               const std::vector<double>& y1, const std::vector<double>& y2,
                               const std::string& title = " ",
                               const std::string& labelx = " ", const std::string& labely = " ", 
                               const std::string& legend1 = "Real", const std::string& legend2 = "Imag",
                               const std::string& linestyle = "-"){
        plt::figure(number);
        plt::named_plot(legend1, x, y1, linestyle);
        plt::named_plot(legend2, x, y2, linestyle);
        plt::title(title);
        plt::xlabel(labelx);
        plt::ylabel(labely);
        plt::legend(); 
    }
};

// Drude model:
// ε(ω) = ε∞ − ωp² / (ω² + iγω)
std::complex<double> drude_eps(double omega , double eps_inf , double omega_p , double gamma){
    std::complex<double> denom = {omega*omega, gamma*omega};
    return eps_inf - (omega_p * omega_p)/denom;
}

// find error of data vs drude model
double computeError (const std::vector<double>& omega , double eps_inf , double omega_p , double gamma, const std::vector<double>& eps1_data, const std::vector<double>& eps2_data){
    std::complex<double> eps_model;
    double error = 0.0;
    for (size_t i = 0 ; i < omega.size() ; i++){
        if (omega[i] < omega_min || omega[i] > omega_max)// Frequency window where Drude model is valid
        continue;
    eps_model = drude_eps(omega[i] , eps_inf , omega_p , gamma);

    // Normalized least-squares error:
    // Σ [ (Re_model − Re_data)² + (Im_model − Im_data)² ] / (|ε_data|²)
    double real_error = std::real(eps_model) - eps1_data[i];
    double img_error = std::imag(eps_model)  - eps2_data[i];

    double norm = eps1_data[i]*eps1_data[i] + eps2_data[i]*eps2_data[i] + 1e-12;
    error += (real_error*real_error + img_error*img_error) / norm;
    }
    return error;
}


int main() {

    // Default for this version of the code.
    PlotLevel plotLevel = PlotLevel::Advanced;  // Change Advanced to Basic to display the version 1 figures.

    Material Ag ("Silver");
    Ag.loadData("data/Ag_Palik_400-900nm.txt");
    std::string name = Ag.getName();
    std::vector<double> wl = Ag.getWavelength();
    std::vector<double> n = Ag.getN();
    std::vector<double> k = Ag.getK();
    std::vector<double> energy = Ag.getEnergy();
    std::vector<double> omega = Ag.getOmega();

    std::pair<std::vector<double>, std::vector<double>> eps = Ag.computeEpsilon();
    std::vector<double> eps1_data = eps.first;
    std::vector<double> eps2_data = eps.second;
    
    // Grid search over plasma frequency and damping rate
    // to minimize squared error between experimental and model permittivity
    double best_error = 1e300;
    double best_omega_p = 0.0;
    double best_gamma = 0.0;
    for (double omega_p = omega_p_min ; omega_p < omega_p_max ; omega_p+=domega_p){
        for (double gamma = gamma_min ; gamma < gamma_max ; gamma+=dgamma){
            double err = computeError(omega, eps_inf, omega_p, gamma, eps1_data, eps2_data);
            if (err < best_error){
                best_error = err;
                best_omega_p = omega_p;
                best_gamma = gamma;
            }
        }
    }

    // Reporting best parameters found in the grid search
    std::cout << "\n***Drude model***\nFitted only within "
          << omega_min << " < omega < " << omega_max << " rad/s with the following fitting parameters:\n";
    std::cout << "omega_p: " << best_omega_p << "  rad/sec \n";
    std::cout << "gamma: " << best_gamma << "  1/s \n";

    std::cout << "Best normalized error is : " << best_error << '\n';
    
    // Calculating the model permittivities based on the best fitting parameters
    std::vector<std::complex<double>> eps_model(omega.size());
    for(size_t i = 0; i<omega.size(); i++){
        eps_model[i]= drude_eps(omega[i], eps_inf, best_omega_p, best_gamma);
    }

    std::vector<double> eps1_model(eps_model.size());
    std::vector<double> eps2_model(eps_model.size());
    for (size_t i = 0; i < omega.size(); i++){
    eps1_model[i] = eps_model[i].real();
    eps2_model[i] = eps_model[i].imag();
    }

    // Plotting the results
    if (plotLevel == PlotLevel::Advanced) {
        Plot::dispersionPlot(1, energy, eps1_model, eps2_model,
                           "Drude Fit vs Palik Data (" + name + ")" + " ",
                           "energy  (eV)", "Permittivity (ε)",
                           "ε₁ Drude fit", "ε₂ Drude fit",
                           "--");


        Plot::dispersionPlot(1, energy, eps1_data, eps2_data,
                           "Drude Fit vs Palik Data (" + name + ")" + " ",
                           "energy  (eV)", "Permittivity (ε)",
                           "ε₁ data", "ε₂ data");
    }

    if (plotLevel == PlotLevel::Basic) {   
        Plot::dispersionPlot(1, wl, n, k,
                            "Refractive Index of " + name + " (Palik, 400–900 nm)",
                            "Wavelength λ (nm)", "Refractive index (n)",
                            "n data", "k data");

        Plot::dispersionPlot(2, wl, eps1_data, eps2_data,
                            "Complex Permittivity of " + name + " (Palik, 400–900 nm)",
                            "Wavelength λ (nm)", "Permittivity (ε)",
                            "ε₁ data", "ε₂ data");
    }

    std::cout << "\nDispersion analysis complete. Figures displayed successfully.\n"
              << "*************************************************************";
    plt::show();

    return 0;
}