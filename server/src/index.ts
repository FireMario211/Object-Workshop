import express from 'express'
import bodyParser from 'body-parser'
import oRouter from './controllers/objects';
import uRouter from './controllers/user';
import sRouter from './controllers/staff';
import morgan from 'morgan';
import { rateLimit } from 'express-rate-limit'
import 'dotenv/config'

const app = express();
const port = 3000;

app.use(bodyParser.json({ limit: '40mb' }));

app.set("trust proxy", 1);

app.use(express.raw({ limit: '40mb' }));

if (!process.env.PRODUCTION) {
    app.use(morgan('combined'));
}

app.all("/", (_, res) => {
    res.sendStatus(200)
})

const limiter = rateLimit({
	windowMs: 3 * 60 * 1000,
	limit: 500,
	standardHeaders: 'draft-7',
	legacyHeaders: false,
    message: { error: "You are ratelimited! Please wait 3 minutes." }
})

app.use(limiter)

app.use(uRouter);
app.use(oRouter);
app.use(sRouter);

app.listen(port, () => { 
  console.log(`Server is running on port @${port}`);
})
