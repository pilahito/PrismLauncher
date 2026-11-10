# SPDX-License-Identifier: GPL-3.0-only
# Instala los temas conservando cualquier tema que el usuario ya haya modificado.
param([Parameter(Mandatory=$true)][string]$CarpetaTemas)
$ErrorActionPreference = 'Stop'
$origen = Join-Path $PSScriptRoot 'community-themes\themes'
New-Item -ItemType Directory -Path $CarpetaTemas -Force | Out-Null
Get-ChildItem -LiteralPath $origen -Directory | ForEach-Object {
    $destino = Join-Path $CarpetaTemas $_.Name
    if (-not (Test-Path -LiteralPath $destino)) {
        Copy-Item -LiteralPath $_.FullName -Destination $destino -Recurse
    }
}
