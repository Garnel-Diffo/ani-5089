// Exercice 4 : les six symptomes, et ce qu'aucun ne dit.
//
// Le programme remonte du symptome observe a l'oubli qui l'a cause. Aucun des six symptomes ne
// dit "vous avez oublie quelque chose a la fin de la trame" : il faut les avoir lus une fois pour
// les reconnaitre.
//
// La regle :
//   1. Chaque symptome connu a exactement une cause :
//        BLOQUE_AU_WAIT        : FIN DE TRAME
//        PAS_DE_REPRISE        : FIN SANS RENDU
//        OEIL_NOIR             : CHAINE MANQUANTE
//        TREMBLEMENT           : MAUVAIS INSTANT
//        CORRECTION_DE_TRAVERS : POSE FRAICHE
//        SCENE_DECALEE         : MAUVAIS ESPACE
//   2. Tout autre symptome donne la cause INCONNU.
//   3. DIAGNOSTIQUES compte les symptomes reconnus, INCONNUS les autres.
//   4. Un meme symptome peut revenir plusieurs fois, et se compte a chaque fois.
//
// Les causes s'ecrivent en majuscules, avec des espaces ; les symptomes, eux, ont des soulignes.
// La ligne INCONNUS s'affiche meme quand elle vaut zero.
//
// Entree : un entier N, puis N lignes, chacune portant un symptome.
// Sortie : une ligne par symptome (le symptome, puis la cause), puis `DIAGNOSTIQUES n` et
// `INCONNUS n`.

#include <iostream>
#include <map>
#include <string>

int main() {
    // Les six symptomes connus et leur cause : la table la plus utile du chapitre.
    const std::map<std::string, std::string> causes = {
        { "BLOQUE_AU_WAIT", "FIN DE TRAME" },
        { "PAS_DE_REPRISE", "FIN SANS RENDU" },
        { "OEIL_NOIR", "CHAINE MANQUANTE" },
        { "TREMBLEMENT", "MAUVAIS INSTANT" },
        { "CORRECTION_DE_TRAVERS", "POSE FRAICHE" },
        { "SCENE_DECALEE", "MAUVAIS ESPACE" },
    };

    int nombre = 0;
    std::cin >> nombre;

    int diagnostiques = 0;
    int inconnus = 0;
    for (int i = 0; i < nombre; ++i) {
        std::string symptome;
        if (!(std::cin >> symptome)) {
            break;
        }
        const auto trouve = causes.find(symptome);
        if (trouve != causes.end()) {
            std::cout << symptome << ' ' << trouve->second << '\n';
            ++diagnostiques;
        } else {
            std::cout << symptome << " INCONNU\n";
            ++inconnus;
        }
    }

    std::cout << "DIAGNOSTIQUES " << diagnostiques << '\n';
    std::cout << "INCONNUS " << inconnus << '\n';
    return 0;
}
