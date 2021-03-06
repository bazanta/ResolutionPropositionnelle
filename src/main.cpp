#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <getopt.h>

#include "../headers/methods.h"

using namespace std;

void print_help() {
  cout << "usage: exe -o 10 -c 12\n"
    "\t-f, --fichier, fichier contenant les contraintes d'un problème\n"
    "\t-m, --methode brute (1), efficace (2), brute OpenMP (3), brute MPI (4), efficace OpenMP (5)\n"
    "\t-d, --afficher les solutions\n"
    "\t-h, --help, affiche cette aide\n" << endl;
}

int main(int argc, char* argv[]) {
  // Initialisation des variables
  string file_name;
  unsigned int method = 1;
  vector<string> variables;
  vector<unsigned int> dimensions;
  vector<Constraint> constraints;
  bool display_solutions = false;
  vector<ull> solutions;
	vector<bool*> solutions_recursif;
  int opt, indice;

  // Traitement des options
  struct option long_option[] =
  {
    {"file", required_argument, NULL, 'f'},
    {"method", required_argument, NULL, 'm'},
    {"display solutions", no_argument, NULL, 'd'},
    {"help", no_argument, NULL, 'h'},
    {0,0,0,0}
  };

  while ((opt = getopt_long(argc, argv, "hf:m:d", long_option, &indice)) != -1) {
    switch (opt) {
    case 'f':
      file_name = optarg;
      break;
    case 'm':
      method = atoi(optarg);
      break;
    case 'd':
      display_solutions = true;
      break;
    case 'h':
      print_help();
      break;
    default:
      print_help();
      exit(EXIT_FAILURE);
    }
  }

  // Analyse du fichier contenant les contraintes
	ifstream fichier(file_name);

	if (fichier.is_open()) {
    string ligne;
		while (getline(fichier, ligne)) {
      int indice = 0;

      // Esquive des espaces
      while (ligne.length() > 0 && ligne[indice] == ' ') {
        ++indice;
      }

      // Traitement des mots-clé
      if (ligne.length() > 0 && ligne[indice] != '#') {
        if (ligne[indice] == 'v') {
          indice += 2;
          int etape = 1;
          string variable;
          string valeur;
          for (unsigned int i = indice; i < ligne.length(); ++i) {
            if (ligne[i] == '#') break;
            if (ligne[i] == ' ' && i > 0 && ligne[i-1] != ' ') etape++;
            else {
              if (etape == 1) variable += ligne[i];
              if (etape == 2) valeur += ligne[i];
            }
          }
          variables.push_back(variable);
          dimensions.push_back(stoi(valeur));
        } else if (ligne[indice] == 'c') {
          indice += 2;
          int etape = 1;
          bool signe = true;
          string min;
          string max;
          string variable;
          for (unsigned int i = indice; i < ligne.length(); ++i) {
            if (ligne[i] == '#') break;
            if (ligne[i] == ' ' && i > 0 && ligne[i-1] != ' ') etape++;
            else {
              if (etape == 2 && signe == true && min.compare("neg") == 0) {
                signe = false;
                min = "";
                etape = 1;
              }
              if (etape == 1) min += ligne[i];
              if (etape == 2) max += ligne[i];
              if (etape == 3) variable += ligne[i];
            }
          }
          ptrdiff_t indice_contrainte = find(variables.begin(), variables.end(), variable) - variables.begin();
          constraints.push_back(Constraint(signe, stoi(min), stoi(max), indice_contrainte));
        }
      } // Fin "si la ligne est non vide et ne commence pas par #"
    }
		fichier.close();
	} else {
		cout << "Impossible d'ouvrir le fichier " << argv[1] << endl;
		return EXIT_FAILURE;
	}

  // Application de la méthode de calcul
  switch (method) {
    case 1: // Méthode brute
      solver_brut(solutions, dimensions, constraints);    
      break;
		case 2: // Méthode efficace
			{
				unsigned int solutions_length = accumulate(dimensions.begin()+1, dimensions.end(), dimensions[0], multiplies<int>());
				bool* matriceVide = new bool[solutions_length];
        for (unsigned int i = 0; i < solutions_length; ++i) matriceVide[i] = 0; // initialiser la matrice avec des 0
				solutions_recursif = solver_efficace(matriceVide, 0, solutions_length, dimensions, constraints);
			}
      break;
    case 3: // Méthode brute parallélisée avec OpenMP
      solver_brut_openMP(solutions, dimensions, constraints);
      break;
    case 4: // Méthode brute parallélisée avec MPI
      solver_brut_mpi(solutions, dimensions, constraints);
      break;
		case 5: // Méthode efficace parallélisée avec OpenMP
			{
				unsigned int solutions_length = accumulate(dimensions.begin()+1, dimensions.end(), dimensions[0], multiplies<int>());
				bool* matriceVide = new bool[solutions_length];
        for (unsigned int i = 0; i < solutions_length; ++i) matriceVide[i] = 0; // initialiser la matrice avec des 0
				solutions_recursif = solver_efficace_openMP(matriceVide, 0, solutions_length, dimensions, constraints);
			}
		  break;
    default:
      break;
  }

	// Affichage des solutions et du nombre de solutions
	if (method == 2 || method == 5) {		
		if (display_solutions) print_solutions_recursif(solutions_recursif, dimensions);
    if (solutions_recursif.size() == 0) cout << "INSATISFIABLE" << endl;
    else cout << "SATISFIABLE" << endl;
		cout << solutions_recursif.size() << " solutions" << endl;
    for (auto sol : solutions_recursif) delete[] sol;
	} else {
		if (display_solutions) print_solutions(solutions, dimensions);
    if (solutions.size() == 0) cout << "INSATISFIABLE" << endl;
    else cout << "SATISFIABLE" << endl;
		cout << solutions.size() << " solutions" << endl;
	}

  return EXIT_SUCCESS;
}
