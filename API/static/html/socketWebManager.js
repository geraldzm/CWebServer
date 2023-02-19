console.log("SocketWebManager.js loaded");
let currentQuestion = null;


// get the params from the url
function getParams() {
    const params = {};
    const urlParams = new URLSearchParams(window.location.search);
    for (let p of urlParams) {
        params[p[0]] = p[1];
    }
    return params;
}

console.log(getParams());
const urlParams = getParams();
// // websocket
// validate state
let ws = new WebSocket("ws:172.19.40.6:2201/web-socket?quizID=" + urlParams['quizcode'] +"&username="+urlParams['username'] , ["myprotocol"]);
// let ws = new WebSocket("ws:localhost:2201/web-socket?quizID=" + urlParams['quizcode'] +"&username="+urlParams['username'] , ["myprotocol"]);

// // Connection opened
ws.addEventListener("open", (event) => {
    console.log("Socket opened!!! :,)");
});

ws.addEventListener("error", (error) => {
    console.log("Socket error: ", error);
});

ws.addEventListener("close", (event) => {
    console.log("Socket closed: ", event);
});

function onSubmittedAnswer() {
    console.log("Submitted answer");
    // get all inputs with name "answer"
    let inputs = document.getElementsByName("answer");
    const selectedAnswers = [];
    // for each input
    for (let input of inputs) {
        // if it is checked
        if (input.checked === true) {
            // add it to selected answers
            selectedAnswers.push(input.value);
        }
    }

    if (selectedAnswers.length > 0 && currentQuestion !== null) {
        console.log(selectedAnswers);
        // send answers to server
        const ms = currentQuestion.id + "," + selectedAnswers.join(",");
        ws.send(ms);

        // set hidden to true of button with id "submitAnswer"
        document.getElementById("submitAnswer").hidden = true;
    }
}

// set question
function setQuestionToBody(question) {
    console.log("Setting question to body");
    console.log(question);
    currentQuestion = question;

    // question will be of the form: {
    //     "id": 2,
    //     "question": "What is the capital of Italy?",
    //     "questionType": "MULTIPLE_CHOICES", // or "SINGLE_CHOICE"
    //     "video": "https://archive.org/download/SampleVideo1280x7205mb/SampleVideo_1280x720_5mb.mp4", // could be null
    //     "image": "https://www.google.com/images/branding/googlelogo/2x/googlelogo_color_272x92dp.png", // could be null
    //     "audio": "https://www.soundhelix.com/examples/mp3/SoundHelix-Song-1.mp3", // could be null
    //     "points": 2,
    //     "time": 6,
    //     "answers": [
    //         {
    //             "id": 6,
    //             "text": "Paris"
    //         },
    //         {
    //             "id": 7,
    //             "text": "Rome"
    //         },
    //         {
    //             "id": 8,
    //             "text": "Berlin"
    //         }
    //     ]
    // }

    // get html element with id "questionTxt"
    let questionTxt = document.getElementById("questionTxt");
    // set question text to question.question
    questionTxt.innerHTML = question.question + "(" + question.points + " points)";

    //get html element with id "questionType"
    let questionType = document.getElementById("questionType");
    // set question type to question.questionType
    questionType.innerHTML = question.questionType === "SINGLE_CHOICE" ? "Respuesta única" : "Respuesta Múltiple";

    // get div with id "answers"
    let answersDiv = document.getElementById("answers");
    // clear answers div
    answersDiv.innerHTML = "";

    // if there is a video
    let media = null;
    if (question.video) {
        // create video element
        media = document.createElement("video");
        media.src = question.video;
        // set autoplay
        media.autoplay = true;
        // set controls
        media.controls = true;
        // set max dimensions
        media.style.maxWidth = "100%";
        media.style.maxHeight = "50%";
    } else if (question.image) {
        // create img element
        media = document.createElement("img");
        media.src = question.image;
        // set dimensions
        media.width = 400;
        media.height = 300;
    } else if (question.audio) {
        // create audio element
        media = document.createElement("audio");
        media.src = question.audio;
    }

    // add media to answers div
    if (media !== null) {
        answersDiv.appendChild(media);
        answersDiv.appendChild(document.createElement("br"));
    }

    // get question type
    let buttonType = question.questionType === "SINGLE_CHOICE" ? "radio" : "checkbox";

    // for each answer
    for (let answer of question.answers) {
        // create input element
        let input = document.createElement("input");
        input.type = buttonType;
        input.name = "answer";
        input.value = answer.id;
        input.id = "answer" + answer.id;
        // create label element
        let label = document.createElement("label");
        label.htmlFor = "answer" + answer.id;
        label.innerHTML = answer.text;
        // create br element
        let br = document.createElement("br");

        // append input, label and br to answers div
        answersDiv.appendChild(input);
        answersDiv.appendChild(label);
        answersDiv.appendChild(br);
    }

    // set hidden to false of button with id "submitAnswer"
    document.getElementById("submitAnswer").hidden = false;

    const timerBar = document.getElementById("timerBar");
    timerBar.classList.remove("round-time-bar");
    timerBar.offsetWidth;
    timerBar.classList.add("round-time-bar");
    timerBar.hidden = false;
    timerBar.style.setProperty("--duration", question.time);
}

function finishQuiz(data) {
    console.log("Finishing quiz");
    console.log(data);

    // data is of the form: { finalScore: 10, ranking: [ { username: "user1", score: 10, position: 1 }, { username: "user2", score: 5, position: 2 } ] }

    // set question text to "Quiz finished"
    document.getElementById("questionTxt").innerHTML = "El quiz ha terminado";

    // get div with id "answers"
    document.getElementById("answers").innerHTML = "";

    // get div with id "questionType"
    document.getElementById("questionType").innerHTML = "";

    // hide all
    document.getElementById("question").hidden = true;

    // get score
    document.getElementById("score").innerHTML = "Mi puntuación final: " + data.finalScore;

    // get div ranking
    let rankingDiv = document.getElementById("ranking");
    rankingDiv.innerHTML = "";
    // create a table for the ranking with headers "Posición", "Usuario" and "Puntuación"
    let table = document.createElement("table");
    let tr = document.createElement("tr");
    let th1 = document.createElement("th");
    th1.innerHTML = "Posición";
    let th2 = document.createElement("th");
    th2.innerHTML = "Usuario";
    let th3 = document.createElement("th");
    th3.innerHTML = "Puntuación";

    tr.appendChild(th1);
    tr.appendChild(th2);
    tr.appendChild(th3);

    table.appendChild(tr);

    // for each user in ranking
    for (let i = 0; i < data.ranking.length; i++) {
        let user = data.ranking[i];
        // create a row with the position, username and score
        tr = document.createElement("tr");
        let td1 = document.createElement("td");
        td1.innerHTML = user.position;
        let td2 = document.createElement("td");
        td2.innerHTML = user.username;
        let td3 = document.createElement("td");
        td3.innerHTML = user.score;

        tr.appendChild(td1);
        tr.appendChild(td2);
        tr.appendChild(td3);

        table.appendChild(tr);
    }

    // add table to ranking div
    rankingDiv.appendChild(table);
    rankingDiv.hidden = false;

    // set button with id "submitAnswer" to hidden false, change the text to "Volver a jugar" and add an onclick event to go to the home page
    document.getElementById("goHome").hidden = false;
}

function goHomeButtonAction() {
    window.location.href = "/";
}

ws.addEventListener("message", (event) => {
    console.log("Socket message: ", event.data);
    const json = JSON.parse(event.data);
    const eventType = json.type;
    const eventBody = json.data;

    switch (eventType) {
        case "CHANGE_QUESTION":
            setQuestionToBody(eventBody);
        break;
        case "FINISH_QUIZ":
            console.log("FINISH_QUIZ");
            finishQuiz( json.data);
        break;
        case "CHANGE_STATUS":
            // set <h1> with id title

            let ms = 'Unkonwn message';
            switch (eventBody) {
                case "QUIZ_HANDLER_WAITING_FOR_ADMIN":
                    ms = "Esperando a que el administrador inicie el quiz";
                break;
                case "QUIZ_HANDLER_INITIALIZED":
                    ms = "";//"El quiz ha iniciado";
                    const score = document.getElementById("score");
                    score.innerHTML = "Score: 0";
                    score.hidden = false;

                break;
                case "QUIZ_HANDLER_ENDED":
                    ms = "El quiz ha terminado";
                    break;
            }

            document.getElementById("title").innerHTML = ms;
        break;
        case "UPDATE_SCORE":
            document.getElementById("score").innerHTML = "Score: " + eventBody;
            break;
        default:
            console.log("Unknown event type: ", eventType);
            console.log(event);
    }
});

