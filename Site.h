/*
PROGMEM is used to store the HTML page in flash memory on an ESP32. 
Because the ESP32 has limited RAM, storing large strings in flash memory helps to conserve RAM for other tasks.
*/
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.8/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-sRIl4kxILFvY47J16cr9ZwB07vP4J8+LH7qKQnuqkuIAvNWLzeN8tE5YBujZqJLB" crossorigin="anonymous">
    <title>WebServer Alimentador Automatico</title>  

    <style>
        body, html{
            text-align: center;
            width: 100%;
            height: 100%;
        }
        .infos{
            display: flex;
            justify-content: space-around;
        }
        .colunas-botoes{
            display: flex;
            flex-direction: column;
            width: 100%;
        }
        .linhas-botoes{
            display: flex;
            margin-top: 50px;
        }
        .metade{
            width: 50%;
        }
        .botao-quadrado{
            height: 20vh;
            width: 20vh;
        }
    </style>
  </head>
  <body>
    <h1>Alimentador automatico 2000</h1>

    <div class="infos mt-4">
        <div>
            <h3>Quantidade atual de ração:</h3>
            <p>87%</p>
        </div>
        <div>
            <h3>Proximo horário de alimentação:</h3>
            <P>14h</P>
        </div>
    </div>


    <div class="colunas-botoes">

        <div class="linhas-botoes">
            <div class="metade">
                <button class="btn btn-outline-success botao-quadrado">
                    <img src="" alt="pouca ração">
                </button>
            </div>
            <div class="metade">
                <button class="btn btn-outline-success botao-quadrado">
                    <img src="" alt="quantidade normal de ração">
                </button>
            </div>
        </div>
        
        <div class="linhas-botoes">
            <div class="metade">
                <button class="btn btn-outline-success botao-quadrado">
                    <img src="cheio.png" alt="muita ração">
                </button>
            </div>
            <div class="metade">
                <button class="btn btn-outline-success botao-quadrado">
                    <img src="" alt="quantidade personalizada">
                </button>
            </div>
        </div>
       

    </div>
   
            
        
  </body>
</html>
)rawliteral";