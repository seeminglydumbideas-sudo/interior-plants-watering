#!/bin/bash

# Verification des arguments
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <openscad_file.scad> <output_directory>"
    exit 1
fi

SCAD_FILE="$1"
OUTPUT_DIR="$2"

if [ ! -f "$SCAD_FILE" ]; then
    echo "Erreur: Le fichier '$SCAD_FILE' n'existe pas."
    exit 1
fi

# Création du dossier de destination s'il n'existe pas
mkdir -p "$OUTPUT_DIR"

# Génération d'un timestamp (Format: YYYYMMDD_HHMMSS)
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
BASENAME=$(basename "$SCAD_FILE" .scad)
OUTPUT_FILE="${OUTPUT_DIR}/${BASENAME}_rendered_${TIMESTAMP}.png"

echo "📸 Génération du rendu OpenSCAD en cours..."
echo "Fichier de sortie : $OUTPUT_FILE"

translate="0,0,0"
rotate="60,0,30"
distance="300"

# Commande OpenSCAD via Flatpak avec vos paramètres de caméra
flatpak run org.openscad.OpenSCAD -o "$OUTPUT_FILE" \
  --imgsize=1500,1000 \
  --projection=p \
  --camera="${translate},${rotate},${distance}" \
  --colorscheme="Tomorrow Night" \
  "$SCAD_FILE"

# Vérification du statut de la commande
if [ $? -eq 0 ]; then
    echo "✅ Rendu terminé avec succès !"
else
    echo "❌ Erreur lors de la génération du rendu. Assurez-vous que l'exécutable 'openscad' est bien installé et accessible via Flatpak."
fi
