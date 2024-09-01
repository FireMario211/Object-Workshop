// Prism Menu Server 
import express, { Request, Response, NextFunction } from 'express'
import bodyParser from 'body-parser'
import oRouter from './controllers/objects';
import uRouter from './controllers/user';
import morgan from 'morgan';
import { rateLimit } from 'express-rate-limit'

const app = express();
const port = 3000;

app.use(bodyParser.json());

app.set("trust proxy", 1);

app.use(express.raw({ limit: '80mb' }));

app.use(morgan('combined'));

app.all("/", (_, res) => {
    res.send("hi my name is firee")
})
/*
    auto ref = CCSprite::create("fanmade3.png"_spr);
    //this->addChild(ref);
    ref->setOpacity(255 / 4);
    ref->setPosition({285, 160});
    ref->setScale(1.29F);
*/
const delay = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));

const simulateLatency = (delay: number) => {
  return (req: Request, res: Response, next: NextFunction) => {
    setTimeout(() => {
      next();
    }, delay);
  };
};

const limiter = rateLimit({
	windowMs: 3 * 60 * 1000, // 15 minutes
	limit: 200, // Limit each IP to 100 requests per `window` (here, per 15 minutes).
	standardHeaders: 'draft-7', // draft-6: `RateLimit-*` headers; draft-7: combined `RateLimit` header
	legacyHeaders: false, // Disable the `X-RateLimit-*` headers.
	// store: ... , // Redis, Memcached, etc. See below.
    message: { error: "You are ratelimited!" }
})

// Apply the rate limiting middleware to all requests.
app.use(limiter)

//app.use(simulateLatency(100));

app.use(uRouter);
app.use(oRouter);

app.listen(port, () => { 
  console.log(`Server is running on port @${port}`);
})
