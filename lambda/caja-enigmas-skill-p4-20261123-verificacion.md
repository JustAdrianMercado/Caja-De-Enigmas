# Paquete local P4, 23/11/2026

Artefacto: `caja-enigmas-skill-p4-20261123.zip`, destinado a
`caja-enigmas-skill`, Python 3.12, Linux x86_64, región `us-east-1`.
Handler: `lambda_function.lambda_handler`. No se cargó en AWS.
El catálogo de Lucía y su adaptador siguen pendientes; producción conserva
los proveedores sin configurar y explica que las pistas no están disponibles.
Este ZIP no representa una integración completa de P4.

## Procedencia y contenido

Se revisaron el README, los requirements y la receta de empaquetado anterior, conservada en el historial de Git.
El intento de instalación con pip para CPython 3.12/manylinux2014_x86_64
falló por DNS. Como alternativa se reutilizaron exclusivamente dependencias
del ZIP anterior, comprobando que sus metadatos coinciden con las 14
versiones de `requirements-despliegue.txt`. No se reutilizaron fuentes antiguas.

Dependencias: ask-sdk-core 1.19.0, ask-sdk-model 1.82.0,
ask-sdk-runtime 1.19.0, boto3 1.43.126, botocore 1.43.126,
certifi 2026.10.23, charset-normalizer 3.5.2, idna 3.20,
jmespath 1.1.0, python-dateutil 2.9.0.post0, requests 2.34.2,
s3transfer 0.19.3, six 1.17.0 y urllib3 2.8.0.

Incluye las 26 fuentes actuales: `lambda_function.py` y `textos.py` en la raíz,
`manejadores/` (incluidos `consultar_sensores.py` y `pedir_pista.py`) y
`servicios/` (incluidos Shadow, vinculaciones y publicación de eventos).
Se verificó igualdad byte a byte con las fuentes del checkout.
Conserva modelos de botocore, metadatos, licencias y las CA públicas de
botocore/certifi. Excluye tests, cachés, bytecode, ejecutables Windows,
credenciales, documentación innecesaria y ZIP anteriores. Un escaneo de
patrones, incluyendo modelos comprimidos, no detectó secretos del proyecto;
solo se admitieron las access keys de ejemplo conocidas de los modelos SDK.

## Verificaciones

- Tamaño ZIP: **18 138 364 bytes** (17,30 MiB).
- Descomprimido: **25 921 221 bytes**; **2 847 archivos**, sin duplicados.
- SHA-256: `95a22b62ff81ca7b0e36d3c65f146df46ff106487aed54e00838a90a370e8986`.
- CRC: correcto; permisos POSIX 0644; handler en raíz, sin carpeta `lambda/`.
- Compilación en memoria de **986 módulos Python**, sin errores ni bytecode
  añadido al ZIP. El SDK produce avisos de sintaxis por escapes antiguos.
- Dos binarios ELF de charset-normalizer comprobados estáticamente como
  CPython 3.12, x86_64; etiquetas manylinux y referencias GLIBC hasta 2.14.
- Suite del checkout: **151 pruebas OK**.
- Suite sobre ZIP extraído, Python 3.12.10 con `-I -S -B`:
  **151 pruebas OK**, cero fallos, errores u omitidas; cero intentos de red.
  Orígenes comprobados: módulos y dependencias se importaron del ZIP extraído,
  sin recurrir a site-packages del equipo. Los tests se copiaron al directorio
  de validación después de extraer, nunca al ZIP.
- Arranque separado con SDK real, credenciales ficticias y red bloqueada:
  importación del handler, despacho y GetItem de DynamoDB con Stubber correctos.
  Cero intentos de red; 23 SyntaxWarning del SDK en cada validación aislada.
- ZIP anterior conservado sin cambios; SHA-256:
  `60cf2ee785a03375b7b365eef36f80bb4e60615143376d3f1d3c5ecaa2b46bd7`.
  También se conservó el respaldo anterior existente.

Las comprobaciones se ejecutaron en Windows: no se ejecutaron los binarios
Linux, ni se comprobó el arranque en Amazon Linux/AWS. No se verificaron
configuración, permisos o desencadenadores desplegados. Las etiquetas de
plataforma se contrastaron con la
[documentación de paquetes Lambda](https://docs.aws.amazon.com/lambda/latest/dg/python-package.html).
Queda pendiente integrar el catálogo y validar el comportamiento en el entorno
de destino antes de considerar terminada la integración.

Los scripts y evidencias locales ignorados por Git quedan en
`lambda/paquete/preparar_p4.py`, `lambda/paquete/validar_p4.py` y
`lambda/paquete/validacion-p4-20261123/` (auditoría JSON, inventario, log de
pruebas y resultados aislados). No se modificó código de producción.
