#!/bin/bash

# Se place automatiquement à la racine du projet
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_ROOT"

GENERATOR_SCRIPT="bin/generate-screenshot.sh"

if [ ! -x "$GENERATOR_SCRIPT" ]; then
    echo "Erreur: Le script $GENERATOR_SCRIPT est introuvable ou n'est pas exécutable."
    chmod +x "$GENERATOR_SCRIPT" 2>/dev/null || exit 1
fi

echo "🔍 Recherche des fichiers main.scad..."

find . -type f -name "main.scad" | while read -r SCAD_FILE; do
    OUTPUT_DIR=$(dirname "$SCAD_FILE")
    BASENAME=$(basename "$SCAD_FILE" .scad)
    
    # Cherche le screenshot le plus récent pour ce fichier
    LATEST_SCREENSHOT=$(ls -t "${OUTPUT_DIR}/${BASENAME}_rendered_"*.png 2>/dev/null | head -n 1)
    
    if [ -n "$LATEST_SCREENSHOT" ]; then
        # Vérifie si le SCAD a été modifié plus récemment que le screenshot
        if [ ! "$SCAD_FILE" -nt "$LATEST_SCREENSHOT" ]; then
            echo "⏭️  Ignoré: $SCAD_FILE (Le rendu est déjà à jour par rapport à la date de modification)"
            continue
        fi
    fi
    
    echo "▶️  Génération pour $SCAD_FILE..."
    "$GENERATOR_SCRIPT" "$SCAD_FILE" "$OUTPUT_DIR"
done

echo "✅ Terminé !"
