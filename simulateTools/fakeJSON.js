/**
 * @author Dell
 *
 * 考虑到我们的硬件还没有完成，并且我们希望加快进度，于是就有了这段代码。
 * 这段代码运行于NodeJS，当访问localhost:3035/log/YYYYMMDD时，返回一个JSON。
 * considering that our hardware has not been completed, and we want to speed up the progress, we have this code.
 * this section of the code runs at NodeJS, and returns a JSON when the localhost:3035/log/YYYYMMDD is accessed.
 *
 * 模拟程序：
 * 这个程序用于模拟从智能花盆通过WIFI返回的带有传感器参数的JSON
 * Simulation program:
 * this program is used to simulate JSON with sensor parameters returned from the intelligent flowerpot through WIFI
 *
 * JSON的内容为：
 * The contents of JSON are as follows:
 *
 *
 * { "20140101": [
 * {"time":"0:0","temp":"18.9","lux":"6.6","mois":"44"},
 * {"time":"0:5","temp":"19.5","lux":"6.8","mois":"45"},
 * {"time":"0:10","temp":"19.9","lux":"7.4","mois":"46"},
 * {"time":"0:15","temp":"20.4","lux":"7.5","mois":"47"},
 * 		...
 * 	]}
 *
 */


var http = require("http");

http.createServer(function (req, res) {
    /**
    * 简单的一个路由，用于判断是否请求的路径为“/log/YYYYMMDD”,并将发送假JSON
    * A simple route to determine whether the request path is "/log/YYYYMMDD"
    * AND send false JSON
    */
    if (req.url.substr(0, 5) == "/log/") {
        if (/^([12]\d{3})(0\d|1[0-2])([0-2]\d|3[01])$/.test(req.url.substr(5))) {
            res.writeHead(200, {"Content-Type": "application/json"});
            res.write(randomData(req.url.substr(5)));
            res.end();
        }
        else {
            res.writeHead(404, {"Content-Type": "text/html"});
            res.end("<h1>404</h1>");
        }
    }
    /**
     * 模拟接受表单
     */
    else if (req.url == "/post" && req.method.toLowerCase() == "post") {
        var dataReceive = "";
        req.addListener("data", function (chunk) {
            dataReceive += chunk;
        });

        req.addListener("end", function () {
            console.log(dataReceive.toString());
            res.writeHead(200, {"Content-Type": "text/html"});
            res.end("<h1>OK</h1>");
        })
    }
    else {
        res.writeHead(404, {"Content-Type": "text/html"});
        res.end("<h1>404</h1>");
    }
}).listen(3035, "localhost");


/*
用于生成假json的方法
A method for generating fake JSON
 */
function randomData(date) {
    /**
     * 生成时间、温度、光强、土壤湿度数据
     * Generation time, temperature, light intensity, soil moisture data
     */
    function rTime(i) {
        return Math.floor(i / 12) + ":" + (i % 12) * 5;
    }

    function rTemp(i) {
        return (6 * Math.sin(3.14 * i / 288) + 20 + 2 * Math.random() - 2 * Math.random()).toFixed(1);
    }

    function rLux(i) {
        var flag = 0;
        if (i < 72 || i > 216) {
            flag = 10;
        }
        else {
            flag = 10000;
        }
        return (flag * 0.6 + flag * Math.random() * 0.4).toFixed(1);
    }

    function rMois(i) {
        return Math.floor(50 + 10 * Math.random() - 10 * Math.random() + Math.sin(i / 288) * 5);
    }


    /**
     * 构造假JSON体
     * Structural fake JSON body
     */
    var jsonBody = "\{ \"" + date + "\"\: \[";
//============================================
    for (var i = 0; i < 288; i++) {
        jsonBody += "\{\"time\"\:\"" + rTime(i) +
            "\"\,\"temp\"\:\"" + rTemp(i) +
            "\"\,\"lux\"\:\"" + rLux(i) +
            "\"\,\"mois\"\:\"" + rMois(i) +
            "\"\}"
        if (i != 287) jsonBody += "\,";
    }
//============================================
    jsonBody += "\]}";

    return jsonBody;
}