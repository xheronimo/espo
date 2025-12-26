# Simple wrapper - útil si deseas volver a crear el ZIP sin regenerar fuentes
param([string]$ProjectPath = (Get-Location).Path)

Compress-Archive -Path "$ProjectPath\*" -DestinationPath "$ProjectPath\SCADA_Water_Tank.zip" -Force
Write-Host "?  ZIP creado en $ProjectPath\SCADA_Water_Tank.zip"
