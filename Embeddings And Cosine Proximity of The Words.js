let inputs = [ [ 0, 1 ],  ];
let outputs = [ [1], ];

let words = [ `apple`, `orange` ];

let vtr = `${words[0]}\n`
vtr += `${words[1]}\n`

for (let index = 0; index < inputs.length; index++) {
	vtr +=`\n[${inputs[index][0]}, ${inputs[index][1]}] = ${outputs[index][0]}\n`
}


let embeddings = [];
let threeHundred = 2;
let learningRate = 0.001;
for ( let count = 0; count < words.length; count++ ) {
	embeddings.push ( Array.from ( {length: threeHundred}, _=>Math.random () * 2 - 1));
}

for (let i = 0; i < inputs.length; i++) {
    let input = inputs[i];
    let word1 = input[0];
    let word2 = input[1];
	let currentWords = [word1, word2];
    
	let denominator = 0;
    let N = 0;
	let D = [0, 0];
    for (let index = 0; index < threeHundred; index++) {
        N += embeddings[word1][index] * embeddings[word2][index];
		D[0] += embeddings[word1][index]**2;
		D[1] += embeddings[word2][index]**2;
    }
	denominator = Math.sqrt ( D[0] ) * Math.sqrt ( D[1] );
	
	let cosine = N / denominator;

    vtr += `\n( ( ${embeddings[word1][0].toString().substr(0,6)} x ${embeddings[word2][0].toString().substr(0,6)} ) + ( ${embeddings[word1][1].toString().substr(0,6)} x ${embeddings[word2][1].toString().substr(0,6)} ) ) /  ( sqrt(  ${embeddings[word1][0].toString().substr(0,6)}^2 + ${embeddings[word1][1].toString().substr(0,6)}^2 )sqrt(  ${embeddings[word2][0].toString().substr(0,6)}^2 + ${embeddings[word2][1].toString().substr(0,6)}^2 ) ) = ` + cosine;
	
	vtr += `\n\nN = ${N}`
	
	let derivatives = [
	
		Array.from ( {length: threeHundred}, _=>0 ),
		Array.from ( {length: threeHundred}, _=>0 ),
	];
	
	for ( let wordIndex = 0; wordIndex < 2; wordIndex++ ) {
		for ( let embeddingsIndex = 0; embeddingsIndex < threeHundred; embeddingsIndex++ ) {
			
			// 1. Находим индекс соседнего вектора (если wordIndex = 0, то other = 1, и наоборот)
			let otherWordIndex = wordIndex ^ 1; 

			// 2. Собираем числитель: (e10 * D0^2) - (e00 * N)
			let num = (embeddings[currentWords[otherWordIndex]][embeddingsIndex] * D[wordIndex]) - (embeddings[currentWords[wordIndex]][embeddingsIndex] * N);

			// 3. Собираем знаменатель: D0^2 * D0 * D1
			let den = D[wordIndex] * Math.sqrt(D[wordIndex]) * Math.sqrt(D[otherWordIndex]);

			// 4. Записываем готовую производную
			derivatives[wordIndex][embeddingsIndex] = num / den;
			
		}
	}
}

navigator.clipboard.writeText(vtr);