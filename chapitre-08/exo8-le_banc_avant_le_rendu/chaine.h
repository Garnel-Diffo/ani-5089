// chaine.h : une chaine d'echange, reduite a ce qu'elle est vraiment, de la comptabilite.
//
//   - un anneau de `mImageCount` images, au plus kImagesMax : un tableau a taille FIXE, donc zero allocation
//     (quand le maximum est connu et petit, on n'alloue pas ; quatre suffisent, les runtimes reels en donnent
//     deux ou trois) ;
//   - un seul booleen : une image est-elle empruntee en ce moment ?
//   - deux methodes qui REFUSENT, en retournant false, au lieu de planter ou de laisser faire :
//       AcquireImage refuse le double emprunt, ReleaseImage refuse de rendre ce qui n'a pas ete emprunte ;
//     et Create refuse une chaine plus grande que le maximum.
//
// Les images sont des poignees OPAQUES : des entiers de 64 bits sans type. La chaine ne sait pas ce qu'elles
// designent (une texture Vulkan, un pointeur Direct3D, un entier OpenGL) : elle les range et les rend, elle ne
// les interprete jamais. Et elle ne fabrique aucune image : avec un vrai casque c'est le runtime qui les cree ;
// avec le simulateur, c'est l'application, et la chaine ne fait que les retenir (RegisterImage).
//
// La discipline, en quatre temps :  AcquireImage  ->  rendu  ->  ReleaseImage  ->  soumission.
// Refuser n'est pas planter : une methode qui retourne false laisse le programme decider.

#ifndef CHAINE_H
#define CHAINE_H

#include <cstdint>

class ChaineEchange {
public:
    static constexpr std::uint32_t kImagesMax = 4;

    // Cree la chaine de `nombreImages` images. Refuse (false) zero image, ou plus de kImagesMax : dans ce cas
    // la chaine n'est pas touchee. Une chaine neuve a toutes ses poignees a zero, rien n'est emprunte, et
    // l'anneau pointe sur l'image 0.
    bool Create(std::uint32_t nombreImages) noexcept {
        if (nombreImages == 0 || nombreImages > kImagesMax) {
            return false;
        }
        Destroy();
        mImageCount = nombreImages;
        return true;
    }

    // Detruit la chaine : tout est remis a zero, poignees comprises (c'est pourquoi, apres une recreation,
    // il faut RE-enregistrer les images : le tableau est plein de zeros).
    void Destroy() noexcept {
        for (std::uint32_t i = 0; i < kImagesMax; ++i) {
            mHandles[i] = 0;
        }
        mImageCount = 0;
        mNextImage = 0;
        mAcquise = false;
    }

    bool IsValid() const noexcept { return mImageCount != 0; }
    std::uint32_t GetImageCount() const noexcept { return mImageCount; }
    std::uint32_t GetNextImage() const noexcept { return mNextImage; }
    bool IsAcquired() const noexcept { return mAcquise; }

    // Enregistre la poignee de l'image `index` (fabriquee par l'application). Refuse un index hors anneau.
    bool RegisterImage(std::uint32_t index, std::uint64_t handle) noexcept {
        if (index >= mImageCount) {
            return false;
        }
        mHandles[index] = handle;
        return true;
    }

    // La poignee de l'image `index`, ou 0 ("rien") si l'index est hors anneau ou jamais enregistre.
    std::uint64_t GetImageHandle(std::uint32_t index) const noexcept {
        return index < mImageCount ? mHandles[index] : 0;
    }

    // Temps 1 : "prete-moi une image, laquelle ?". REFUSE le double emprunt : tant que l'image precedente
    // n'est pas rendue, retourne false et ne change RIEN. Sinon, `index` recoit l'image pretee, sur laquelle
    // pointe l'anneau, qu'on dessine seul, et l'emprunt est marque.
    bool AcquireImage(std::uint32_t &index) noexcept {
        if (!IsValid() || mAcquise) {
            return false;
        }
        mAcquise = true;
        index = mNextImage;
        return true;
    }

    // Temps 3 : "j'ai fini, elle est a toi". REFUSE de rendre ce qui n'a pas ete emprunte : retourne false et
    // ne change RIEN. Sinon, rend l'image ET fait avancer l'anneau sur la suivante.
    bool ReleaseImage() noexcept {
        if (!IsValid() || !mAcquise) {
            return false;
        }
        mAcquise = false;
        mNextImage = (mNextImage + 1u) % mImageCount;
        return true;
    }

    // Temps 4 : la soumission affiche la derniere image RENDUE, pas l'image "suivante" sur laquelle pointe
    // maintenant l'anneau : ReleaseImage a deja avance, il faut donc reculer d'un cran. Avec une seule image le
    // modulo ramene toujours a zero et l'erreur est invisible ; avec deux ou plus, elle ne l'est plus.
    std::uint32_t GetLastReleasedIndex() const noexcept {
        return IsValid() ? (mNextImage + mImageCount - 1u) % mImageCount : 0u;
    }

private:
    std::uint64_t mHandles[kImagesMax] = { 0, 0, 0, 0 };
    std::uint32_t mImageCount = 0;
    std::uint32_t mNextImage = 0;
    bool mAcquise = false;
};

#endif // CHAINE_H
