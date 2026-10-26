# Le Projet !
Le but va être de créer une application complète de PasswordManager pour un M5Stick S3. \
-> Le projet sera codé avec PlatformIO \
-> La DA du projet va être simple, clean, minimaliste. Tout doit être à but fonctionnel (voir images) \
-> Le projet doit être suffisamment sécurisé, on stocke tout chiffré dessus, et chiffré d'une bonne manière \
-> Le projet sera réalisé **entièrement** en anglais \
-> Le M5Stick sera en position horizontale, le bouton B vers le dessus, pour un écran plus large.




# Les boutons
On a accès à deux boutons, le bouton A (à l'avant) et le bouton B (sur le côté). \
Dans toute l'application,

- Un appui sur A aura pour effet d'_avancer_
- Un appui sur B aura pour effet de _reculer_
- Un appui long sur A aura pour effet de _valider_ (ce sera parfois A+B)
- Un appui long sur B aura pour effet de _repasser à l'écran précédent_ \
  _(Ceci est l'idée générale, mais chaque effet de chaque bouton sera décrit pour chaque écran, dans la suite du document)_




# Tous les différents écrans accessibles dans le logiciel
Voici tous les différents écrans que l'on pourra atteindre dans le logiciel, avec pour chacun son lien vers les autres, ses boutons, son style, ses actions possibles, etc...
(Sans compter le premier lancement, décrit plus tard)

## 0. Les trucs spéciaux
1. Afficher un message à l'écran, permet d'afficher une info ou une erreur à l'écran, en haut, ça met le type : "Info" ou "Error" et dessous, au centre, la description. Ça reste affiché pendant 2 s avant de redonner la main au code (v1), ou, en fonction des options, reste affiché jusqu'à l'appui d'une touche (v2).

## 1. L'écran de connexion
À l'allumage du M5Stick, on tombe sur cet écran qui va demander le PIN, pour déverrouiller les mots de passe.
### À quoi il ressemble
Au milieu vertical de l’écran, on voit les caractères déjà validés du PIN, côte à côte et centrés horizontalement. À chaque validation, le nouveau caractère s’ajoute à la suite et le groupe se recentre. Si le PIN est long, l’espacement se réduit pour que 12 chiffres tiennent à l’écran.
En bas de l'écran, on voit une rangée de 5 chiffres centrée sur le chiffre actuel, ses voisins sont de plus en plus estompés (ex: 1 2 [3] 4 5)
En haut à droite, on voit une petite icône de batterie qui montre à quel point l'appareil est chargé/s'il est branché (l'icône de batterie doit avoir 5 états, de quasi vide à plein).
### Les actions possibles
- A : incrémente de 1 le caractère en cours sur le PIN (redescend à 0 après 9)
- B : décrémente de 1 le caractère en cours
- B long : retire le dernier caractère tapé
- A long : valide le caractère en cours pour en taper un suivant
- A+B simultanément : valide le PIN pour le tester
  _Si jamais le PIN est mauvais, on écrit un petit message d'erreur (voir 0.1, v1), puis on revient sur l'écran de connexion (en remettant le PIN noté à zéro sur l'écran).
  Si le PIN est bon, on va directement sur l'écran de liste des mots de passe (écran 2)_

## 2. L'écran de liste des mots de passe
Cet écran sert à afficher la liste des mots de passe et autres notes sécurisées (carte de crédit, ...).
On pourra, entre autres, depuis cet écran en choisir un pour avoir plus d'infos.
Si jamais on revient sur cet écran depuis un autre, on revient tout en haut de la liste, on ne retient pas où on était.
### À quoi il ressemble
On a une liste verticale d'items, le premier est le seul spécial.
Le premier item est un bouton Add pour ajouter un nouveau mot de passe depuis le M5Stick. Le reste de la liste est constitué de tous les mots de passe stockés (et cartes de crédit, ... évidemment, j'arrêterai de le dire). Les MDP sont triés dans l'ordre de leur utilisation, en haut ceux qu'on a utilisés en dernier, et en bas ceux qui sont inutilisés depuis longtemps.
_(Logique, mais si on descend la liste, ça ne fait que descendre le focus et si le focus arrive en bas de l'écran, ça fait monter la liste, similaire pour monter)_
### Les actions possibles
- A : descend de 1 (si on est tout en bas, va tout en haut)
- B : monte de 1 (si on est tout en haut, va tout en bas)
- B long : amène sur l'écran 3
- A long : si on est sur Add, ça emmène sur l'écran pour créer un mot de passe (écran 5), sinon, ça ouvre l'écran du mot de passe (écran 4)

## 3. L'écran d'application/de redirection
Un simple écran permettant de choisir où aller, dans la liste, dans les paramètres, ...
### À quoi il ressemble
Une liste horizontale de carrés avec une lettre au milieu (S pour Settings, W pour Wi-Fi, Q pour Quit, ...). La liste NE défile PAS pour toujours laisser l'item sélectionné au milieu horizontal de l'écran, elle est statique. Au-dessus de la liste, le nom complet du truc sélectionné, si on est sur Q, ça marquera Quit, par exemple. En haut à droite, la même batterie que sur l'écran 1. \
Voici les différentes options :
1. "Password", l'écran 2
2. "Online" (il faudra trouver un meilleur nom pour cette fonctionnalité), l'écran 6
3. "Settings", l'écran 7
4. "Quit", éteint l'appareil
### Les actions possibles
- A : avance de 1
- B : recule de 1
- B long : éteint le M5Stick
- A long : ouvre l'écran lié à la sélection actuelle.

## 4. Écran d'un mot de passe ou autre champ de données
Cet écran permet de voir les champs obfusqués et non obfusqués pour pouvoir les voir et les saisir à la main, ou les saisir avec l'USB HID.
### À quoi il ressemble
C'est en vertical, pour chaque info, on a son nom et juste en dessous, son texte (écrit totalement pour les champs non obfusqués, ou juste ***** pour les champs obfusqués).
En bas, une option pour supprimer le mot de passe.
### Les actions possibles
- A : descend de 1
- B : monte de 1
- B long : revient sur l'écran de liste des mots de passe (écran 2)
- A long :
- - Si on est sur un champ, à l'aide de (0.1 v2), ça l'affiche entièrement pour pouvoir le lire, ça ne l'affiche pas en obfusqué, évidemment. Puis on revient sur l'écran du mot de passe, en gardant le focus sur l'item cliqué
- - Si on est sur supprimer, affiche à l'aide de (0.1 v1) un avertissement disant qu'il faut appuyer dessus une seconde fois pour valider, si c'est la deuxième fois, on supprime juste le mot de passe et on revient à l'écran 2.
- A+B: Tape le champ actuel en usb hid (affiche grace à 0.1 v1 que c'est bon), même effet que le champ soit obfusqué ou non.

## 5. Écran de création de mot de passe
Cet écran permet de créer un mot de passe directement depuis le M5Stick, non conseillé car pas très pratique, ça doit néanmoins rester possible.\
On va d'abord choisir le nom du mot de passe, puis l'username, puis ça génère un mot de passe automatiquement (on choisira juste sa taille).
### À quoi il ressemble
On écrira ici de la même manière que l'on a écrit le PIN à l'écran 1, mais avec toutes les lettres de l'alphabet en minuscule, puis en majuscule, puis les nombres de 0 à 9, puis l'espace *(Pour l'espace, dans sa case, on ne mettra pas un espace car c'est peu lisible, mais plutôt l'icône de la touche espace)*.
Donc, en haut, on voit le titre de ce que l'on est en train de choisir ("Title" -> "Username/Email" -> "Password size").
- Pour le titre et l'username, on a donc accès à l'alphabet/chiffres/... définis juste au-dessus, en bas de l'écran est écrit ("A+B: validate" et "Long B: Temp").
- Pour le mot de passe, en bas, on a ("A+B: validate" et "Long B: Cancel"), on a juste un chiffre (car pas besoin de plusieurs chiffres, mais juste un nombre entre mot de passe taille minimal et mot de passe taille maximal) au milieu de l'écran que l'on peut mettre entre 0 et la taille maximale d'un mot de passe, il commence sur la taille par défaut d'un mot de passe.
### Les actions possibles
- A : incrémente de 1
- B : décrémente de 1
- B long : retire le dernier caractère tapé, s'il n'y en a pas, on valide comme avec un A long et on prend comme entrée "Temp" au lieu du texte entré.\
Pour Password qui n'a pas plusieurs caractères, revient à l'écran 2 et ne crée pas de mot de passe.
- A long :
- - Si on est sur Title/Username, passe au caractère suivant
- - Si on est sur Password, a le même effet que A+B, ça valide
- A+B : valide, on passe de Title->Username->Password et après Password, ça crée le mot de passe, le met en première position dans l'ordre des mots de passe, et ça nous renvoie à l'écran 2 avec le focus sur ce mot de passe.

## 6. Online ou Wi-Fi (un meilleur nom reste à trouver)
Cet écran ouvrira un Wi-Fi qui permettra d'ouvrir une page web et de gérer les mots de passe depuis son téléphone ou PC.
### À quoi il ressemble
À gauche, un QR Code pour se connecter directement avec son téléphone ; à droite du QR Code, le nom et le mot de passe du Wi-Fi (mot de passe à 6 caractères aléatoires). \
Quand un appareil tente de se connecter, ça nous affiche une info sur l'appareil (son nom si possible ? Sinon, on fera avec son IP), et on peut accepter ou refuser. \
Une fois cet appareil connecté, l'écran change et affiche simplement qu'il est connecté. Quand l'appareil se déconnecte, on revient sur l'écran 3.\
Quelques infos utiles pourront être affichées sur l'écran durant sa connexion avec un appareil. Par exemple, si l'utilisateur valide ses opérations sur son appareil, le M5Stick recevra seulement les modifications et affichera un succès sur cet écran. Chaque nouvelle info affichée écrasera la précédente, et les infos ne restent que 3 s.
### Les actions possibles
- B retire la connexion en cours pour en accepter une autre à la place, on revient au début du processus
- Pour la validation de connexion d'un appareil, A accepte et B refuse (ça doit être écrit)
- B long : revient sur l'écran 3

## 7. Les paramètres
On verra plus tard pour les paramètres, ils devront permettre de choisir un thème, le clavier pour USB HID (AZERTY/QWERTY), ...

# Première connexion
À la première connexion, on tombe sur un écran de bienvenue.
Sur ce premier écran, un qrcode, avec le lien dessous (qui contiendra toutes les explications du produit). Et un petit texte précisant d'appuyer longtemps sur A pour passer à la suite.
Sur cette page, on doit définir le PIN, la page est dans le même style que la page de connexion sauf qu'il y a un texte supplémentaire en haut qui dit "Choose PIN" et un en bas qui dit "A+B: Validate" (couleur mid) -> Si le PIN est trop court ou trop long (plus de 12 chiffres), on affiche l'erreur avec l'intervalle de taille permis (à l'aide de (0.1 v1)).




# UI
### Style
On n'utilisera que deux couleurs, à la manière d'un affichage sur quelque chose comme le Flipper Zero. Ces deux couleurs doivent être stockées dans des variables pour être facilement modifiables, par défaut, ce sera du noir et blanc.
Une troisième et une quatrième couleur seront utilisées, une sorte de mélange entre les deux autres pour imiter une transparence, pour des éléments moins important. On les stockera aussi dans des variabless (un peu transparent: mid, tres transparent: dim).

### Images
Des images du style recherché sont dans 'Explication Projet/ImageRef'
Ces images sont faites sur FigCraft et ont vocation à guider le projet, il ne faut pas beaucoup s'en écarter.


### Code
On passera par un canvas pour un affichage plus fluide. Il faut modifier l'affichage uniquement quand c'est nécessaire. Il ne faut pas négliger les petites optimisations qui permettront de ne pas réafficher l'UI à chaque tour dans la boucle principale.




# Sécurité
### Objectif
Je ne vise pas une sécurité parfaite, mais une bonne sécurité. Tous les mots de passe doivent être chiffrés dans la mémoire. Deviner le PIN en ayant accès à la machine doit être lent et pénible.

### Principe
PIN -> (PBKDF2 + sel) -> **KEK** -> déchiffre -> **Clé maître** -> déchiffre -> **Coffre** (mots de passe)

- La **clé maître** est aléatoire (256 bits) et chiffre le coffre (AES-256-GCM).
- La **KEK** est dérivée du PIN et ne sert qu'à chiffrer la clé maître.
- Le PIN et la KEK ne sont **jamais stockés**.
- Un PIN correct = le tag GCM de la clé maître est valide. Pas de hash du PIN.

### Changer le PIN
Nouveau sel -> nouvelle KEK -> on rechiffre la **même** clé maître. Le coffre n'est pas touché.

### Protections
- PIN de 6 à 12 chiffres.
- Tout est déchiffré en RAM uniquement, et effacé au verrouillage.

### Limite
Si la flash est copiée, seul le coût du PBKDF2 freine les essais hors ligne. La longueur du PIN est donc le facteur clé.

# Stockage des données
Les données seront stockées sous cette forme :
```Vault
  version      u8
  entryCount   u16
  entries[]:
    id           u32   (aléatoire, unique, stable)
    title        string
    fieldCount   u8
    fields[]:
      type       u8
      label      string   (présent uniquement si le type est générique)
      value      string
```

### Entrée
Une entrée est composée de :
- Un identifiant
- Un titre
- Une liste de champs (classiquement, un username/email et un mot de passe)

### Types
| N° | Libellé | Obfusqué | Libellé stocké |
|----|---------|----------|----------------|
| 1 | Username / Email | non | non |
| 2 | Password | oui | non |
| 3 | URL | non | non |
| 4 | Card number | oui | non |
| 5 | Expiry | non | non |
| 6 | CVV | oui | non |
| 7 | Note | non | non |
| 8 | Text | non | oui |
| 9 | Secret | oui | oui |

Quand le libellé n'est pas stocké, le M5Stick le déduit du type (ex. 2 → « Password »), ce qui économise de la place. Sur le site web, on ne peut donner un nom qu'aux champs Text et Secret.

### Création
Pour créer des entrées spéciales, il faudra utiliser la page web, on ne peut créer que des mots de passe classiques depuis le M5Stick.

### Ordre de la liste
Chaque utilisation (A+B ou affichage d'un champ) incrémente un compteur global, l'entrée mémorise sa valeur. \
Ces compteurs sont stockés dans un fichier séparé du coffre pour éviter de le réécrire à chaque consultation.

### Limites de tailles
- Titre: 32
- Valeur: 128
- Libellé 24