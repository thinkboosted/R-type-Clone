-- Fonction utilitaire pour créer un bloc rapidement
-- x,y,z : Position
-- sx,sy,sz : Taille (Scale)
local function creerBloc(x, y, z, sx, sy, sz)
    -- 1. On crée l'entité avec sa position initiale via le composant Transform
    -- Note: On suppose que "Transform" est le nom du composant de position dans ton moteur
    local id = ECS.createEntity({
        Transform = { x = x, y = y, z = z, sx = sx, sy = sy, sz = sz }
    })

    -- 2. On ajoute un visuel (ici un cube de base, assure-toi d'avoir ce fichier)
    -- Si tu n'as pas de cube.obj, le collider invisible fonctionnera quand même pour la physique
    ECS.createMesh(id, "assets/models/cube.obj")
    
    -- Optionnel : Une texture de grille pour le style "Blockout"
    ECS.setTexture(id, "assets/textures/plane_texture.png")

    -- 3. On ajoute la physique
    -- Type "box"
    -- Dimensions sx, sy, sz
    -- Masse = 0 (C'est CRUCIAL : 0 signifie que l'objet est statique/immobile)
    -- Friction = 0.5 (Pour ne pas glisser comme sur de la glace)
    ECS.setCollider(id, "box", sx, sy, sz, 0, 0.5)

    return id
end

-- === CRÉATION DU NIVEAU ===

-- 1. Le Sol (Grand, plat, centré)
-- Position (0, -1, 0), Taille (50, 1, 50)
creerBloc(0, -1, 0, 50, 1, 50)

-- 2. Un Mur devant le joueur
-- Position (0, 1, 10), Taille (4, 2, 1)
creerBloc(0, 1, 10, 4, 2, 1)

-- 3. Une plateforme sur le côté pour sauter dessus
-- Position (5, 0.5, 5), Taille (2, 0.5, 2)
creerBloc(5, 0.5, 5, 2, 0.5, 2)

-- 4. Un escalier rudimentaire (3 marches)
creerBloc(-5, 0, 5,  1, 0.5, 1)
creerBloc(-5, 0.5, 6, 1, 0.5, 1)
creerBloc(-5, 1, 7,   1, 0.5, 1)

ECS.log("Niveau Blockout généré avec succès !")