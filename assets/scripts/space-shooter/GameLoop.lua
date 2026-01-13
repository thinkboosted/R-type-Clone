-- GameLoop.lua - Hello World Edition

local HelloWorldSystem = {}

function HelloWorldSystem.init()
    print("[Lua] Initialisation du Hello World...")

    -- 1. Créer une entité pour le texte
    local textEntity = ECS.createEntity()

    -- 2. Ajouter un composant Transform pour la position
    -- On le place au centre de l'écran (800x600)
    ECS.addComponent(textEntity, "Transform", { x = 250, y = 300, scale = 1.0 })

    -- 3. Créer le texte
    -- Paramètres : id, texte, fontPath, fontSize, isScreenSpace
    ECS.createText(textEntity, "Hello World !", "assets/fonts/arial.ttf", 48, true)

    print("[Lua] Texte 'Hello World !' créé avec l'ID: " .. textEntity)
end

function HelloWorldSystem.update(dt)
    -- Synchronise la position du Transform avec le Renderer
    ECS.syncToRenderer()
end

-- Enregistrement
ECS.registerSystem(HelloWorldSystem)