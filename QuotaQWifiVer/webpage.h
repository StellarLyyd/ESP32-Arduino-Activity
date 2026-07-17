#ifndef WEBPAGE_H
#define WEBPAGE_H

const char HTML_START[] = R"rawliteral(
<html>
<head>
  <title>QuotaQuom</title>

  <style>
    body {
      margin: 0;
      padding: 20px;
      font-family: Times, serif;
      text-align: center;
      background-color: #202124;
      color: white;
    }

    button {
      font-family: Times, serif;
      font-size: 32px;
      padding: 25px 60px;
      width: 150px;
      height: 100px;
      color: 
    }
  </style>
</head>

<body>

<h1>QuotaQuom</h1>

<h2>Encoder:
)rawliteral";

const char HTML_END[] = R"rawliteral(
</h2>

<p><a href="/H"><button>OUT</button></a></p>
<p><a href="/L"><button>IN</button></a></p>

</body>
</html>
)rawliteral";

#endif