# Build Droid Sans Fallback for WFM

## Step 1: Merge Droid Sans Fallback Full and Droid Sans Fallback Legacy

Install [ot-builder-cli](https://www.npmjs.com/package/ot-builder-cli), and then run

```
otb-cli DroidSansFallbackFull.ttf DroidSansFallbackLegacy.ttf --merge --gc -o DroidSansFallbackMerged.ttf
```

## Step 2: Subset Droid Sans Fallback Merged

Install [fonttools](https://pypi.org/project/fonttools/), and then run

```bash
pyftsubset DroidSansFallbackMerged.ttf \
    --unicodes-file=charset/adobe-latin-3.uni --unicodes-file=charset/adobe-cyrillic-1.uni --unicodes-file=charset/adobe-greek-1.uni \
    --unicodes-file=charset/uro.uni --unicodes-file=charset/hangul.uni \
    --unicodes-file=charset/adobe-gb1-2.uni --unicodes-file=charset/adobe-cns1-0.uni --unicodes-file=charset/adobe-japan1-2.uni --unicodes-file=charset/adobe-korea1-1.uni \
    --unicodes-file=charset/cn-general-8105.uni --text-file=charset/jp-freq-2136.txt --text-file=charset/jp-name-863.txt \
    --output-file=DroidSansFallbackWoWG.ttf
```
