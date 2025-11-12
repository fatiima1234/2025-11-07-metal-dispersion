// Metal Dispersion Analyzer
// Author: Fatemeh HadavandMirzaee
// Version 1.0
// Date: 9 November 2025
// Description: A C++ program for base dispersion analysis of metals (e.g., Ag, Au).
// Loads wavelength-dependent refractive index (n) and extinction coefficient (k)
// data from file, computes the complex permittivity ε = ε₁ + iε₂ where
// ε₁ = n² − k² and ε₂ = 2nk, and plots both (n, k) and (ε₁, ε₂) versus wavelength.
// This version focuses solely on material optical properties, forming the foundation
// for later extensions such as Fresnel equation integration.


#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

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
        std::cout << "\nLoaded "<< wavelength_.size() << " data points for " << name_ << " between " << wavelength_.front() << " and " << wavelength_.back() << " nm\n"; 
        }

    // Computes and returns (ε₁, ε₂) from loaded n and k data.
    std::pair<std::vector<double>, std::vector<double>> computeEpsilon() const {
        std::vector<double> e_real, e_imag;
        for (int i = 0 ; i < wavelength_.size() ; ++i){
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

    // Plots two datasets (real and imaginary parts) against wavelength.
    static void dispersionPlot(const int& number,
                               const std::vector<double>& wavelength,
                               const std::vector<double>& n, const std::vector<double>& k,
                               const std::string& title = " ",
                               const std::string& labelx = " ", const std::string& labely = " ", 
                               const std::string& legend1 = "Real", const std::string& legend2 = "Imag"){
        plt::figure(number);
        plt::named_plot(legend1, wavelength, n);
        plt::named_plot(legend2, wavelength, k);
        plt::title(title);
        plt::xlabel(labelx);
        plt::ylabel(labely);
        plt::legend(); 
    }
};


int main() {
    
    Material Ag ("Silver");
    Ag.loadData("data/Ag_Palik_400-900nm.txt");
    std::string name = Ag.getName();
    std::vector<double> wl = Ag.getWavelength();
    std::vector<double> n = Ag.getN();
    std::vector<double> k = Ag.getK();

    Plot::dispersionPlot(1, wl, n, k,
                        "Refractive Index of " + name + " (Palik, 400–900 nm)",
                        "Wavelength λ (nm)", "Refractive index (n)",
                        "n (real part)", "k (imag part)");
    
    std::pair<std::vector<double>, std::vector<double>> eps = Ag.computeEpsilon();
    std::vector<double> eps1 = eps.first;
    std::vector<double> eps2 = eps.second;

    Plot::dispersionPlot(2, wl, eps1, eps2,
                        "Complex Permittivity of " + name + " (Palik, 400–900 nm)",
                        "Wavelength λ (nm)", "Permittivity (ε)",
                        "ε₁ (real part)", "ε₂ (imag part)");

    std::cout << "\nDispersion analysis complete. Figures displayed successfully.\n"
              << "*************************************************************";
    plt::show();

    return 0;
}